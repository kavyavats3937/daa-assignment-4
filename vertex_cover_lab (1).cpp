// DAA Assignment-4, Project 3: Approximation Lab
// Exact (brute-force) Vertex Cover  vs  2-approximation Vertex Cover
// Build : g++ -O2 -std=c++17 vertex_cover_lab.cpp -o vclab
// Run   : ./vclab        (writes summary.csv, graphs14.csv, tight.csv)
#include <bits/stdc++.h>
using namespace std;
using Clock = chrono::steady_clock;
typedef vector<pair<int,int>> EdgeList;

mt19937 rng(2026);
volatile long long sink = 0;

// ---------- Algorithm 1: exact vertex cover by brute force ----------
// Try every subset of vertices (bitmask); keep the smallest subset that covers all edges.
// Time O(2^n * m).  COUNT=true also counts the work done (used for the "steps" column).
template <bool COUNT>
int bruteForceVC(int n, const EdgeList& E, long long* subsets = nullptr, long long* edgeChecks = nullptr) {
    int best = n;
    long long s = 0, c = 0;
    for (unsigned mask = 0; mask < (1u << n); mask++) {
        if (COUNT) s++;
        bool cover = true;
        for (auto& e : E) {
            if (COUNT) c++;
            if (!((mask >> e.first) & 1) && !((mask >> e.second) & 1)) { cover = false; break; }
        }
        if (cover) best = min(best, __builtin_popcount(mask));
    }
    if (COUNT) { *subsets = s; *edgeChecks = c; }
    return best;
}

// ---------- Algorithm 2: 2-approximation (maximal matching) ----------
// Pick any uncovered edge, put BOTH its endpoints in the cover, repeat.  Time O(n + m).
int approxVC(int n, const EdgeList& E, vector<char>& inCover, long long* steps = nullptr) {
    inCover.assign(n, 0);
    int size = 0; long long st = 0;
    for (auto& e : E) {
        st++;
        if (!inCover[e.first] && !inCover[e.second]) {
            inCover[e.first] = inCover[e.second] = 1;
            size += 2;
        }
    }
    if (steps) *steps = st;
    return size;
}

bool isCover(const EdgeList& E, const vector<char>& c) {
    for (auto& e : E) if (!c[e.first] && !c[e.second]) return false;
    return true;
}

// ---------- helpers ----------
EdgeList randomGraph(int n, double p) {
    EdgeList E;
    for (int u = 0; u < n; u++)
        for (int v = u + 1; v < n; v++)
            if (rng() / (double)rng.max() < p) E.push_back({u, v});
    if (E.empty()) E.push_back({0, 1});
    shuffle(E.begin(), E.end(), rng);                 // edge order is random, so the approximation is not biased
    return E;
}
double ms(Clock::time_point a, Clock::time_point b) { return chrono::duration<double, milli>(b - a).count(); }

int main() {
    // ---------- correctness checks on graphs with known answers (stderr) ----------
    {
        auto K = [](int n) { EdgeList E; for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) E.push_back({i, j}); return E; };
        EdgeList star = {{0,1},{0,2},{0,3},{0,4},{0,5}};
        EdgeList path = {{0,1},{1,2},{2,3},{3,4}};
        EdgeList cyc5 = {{0,1},{1,2},{2,3},{3,4},{4,0}};
        bool ok = bruteForceVC<false>(5, K(5)) == 4 && bruteForceVC<false>(6, star) == 1 &&
                  bruteForceVC<false>(5, path) == 2 && bruteForceVC<false>(5, cyc5) == 3;
        if (!ok) { fprintf(stderr, "known-graph check FAILED\n"); return 1; }
        fprintf(stderr, "[OK] brute force gives K5=4, star=1, path P5=2, cycle C5=3 (all known answers)\n");
    }
    int graphsChecked = 0;

    FILE* fs = fopen("summary.csv", "w");
    fprintf(fs, "n,avg_edges,avg_opt,avg_approx,mean_ratio,min_ratio,max_ratio,pct_optimal,brute_ms,approx_us,brute_subsets,brute_edge_checks,approx_steps\n");
    FILE* f14 = fopen("graphs14.csv", "w");
    fprintf(f14, "graph,edges,opt,approx,ratio\n");

    const int GRAPHS = 20;
    for (int n : {10, 12, 14, 16, 18, 20}) {
        double sumM = 0, sumOpt = 0, sumApx = 0, sumRatio = 0, minR = 1e9, maxR = 0, tB = 0, tA = 0;
        double sumChecks = 0, sumSteps = 0; int optimal = 0;
        for (int g = 1; g <= GRAPHS; g++) {
            EdgeList E = randomGraph(n, 0.3);
            // exact: timed, 3 repetitions, average
            int opt = 0; double tb = 0;
            for (int r = 0; r < 3; r++) { auto s = Clock::now(); opt = bruteForceVC<false>(n, E); tb += ms(s, Clock::now()) / 3; }
            long long subsets, checks; bruteForceVC<true>(n, E, &subsets, &checks);
            // approximation: very fast, so repeat 2000 times and divide
            vector<char> cov; long long steps = 0; int apx = 0;
            auto s = Clock::now();
            for (int r = 0; r < 2000; r++) { apx = approxVC(n, E, cov, &steps); sink += apx; }
            double ta_us = ms(s, Clock::now()) * 1000.0 / 2000;
            // checks: cover valid, and approx <= 2 * opt
            if (!isCover(E, cov) || apx > 2 * opt || apx < opt) { fprintf(stderr, "approx check FAILED\n"); return 1; }
            graphsChecked++;
            double ratio = (double)apx / opt;
            sumM += E.size(); sumOpt += opt; sumApx += apx; sumRatio += ratio;
            minR = min(minR, ratio); maxR = max(maxR, ratio); if (apx == opt) optimal++;
            tB += tb; tA += ta_us; sumChecks += checks; sumSteps += steps;
            if (n == 14) fprintf(f14, "%d,%zu,%d,%d,%.3f\n", g, E.size(), opt, apx, ratio);
        }
        fprintf(fs, "%d,%.1f,%.2f,%.2f,%.3f,%.3f,%.3f,%.0f,%.3f,%.3f,%lld,%.0f,%.1f\n", n, sumM / GRAPHS, sumOpt / GRAPHS, sumApx / GRAPHS,
                sumRatio / GRAPHS, minR, maxR, 100.0 * optimal / GRAPHS, tB / GRAPHS, tA / GRAPHS, 1LL << n, sumChecks / GRAPHS, sumSteps / GRAPHS);
        fflush(fs);
    }
    fprintf(stderr, "[OK] on all %d random graphs the approximate cover is valid and opt <= approx <= 2*opt\n", graphsChecked);

    // ---------- worst case for the approximation: complete bipartite graph K(k,k) ----------
    FILE* ft = fopen("tight.csv", "w");
    fprintf(ft, "k,n,edges,opt,approx,ratio\n");
    for (int k = 3; k <= 8; k++) {
        EdgeList E; int n = 2 * k;
        for (int i = 0; i < k; i++) for (int j = 0; j < k; j++) E.push_back({i, k + j});   // fixed (not shuffled) order
        vector<char> cov; int apx = approxVC(n, E, cov);
        int opt = bruteForceVC<false>(n, E);
        fprintf(ft, "%d,%d,%zu,%d,%d,%.3f\n", k, n, E.size(), opt, apx, (double)apx / opt);
    }
    fclose(fs); fclose(f14); fclose(ft);
    return 0;
}
