#define _CRT_SECURE_NO_WARNINGS 

#include <iostream>
#include <vector>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <locale>
#include <random>   // Per Miller-Rabin
#include <limits>   // Per cin.ignore

typedef unsigned long long u64;

// --- 1. FUNZIONI MATEMATICHE (Miller-Rabin) ---
u64 mul_mod(u64 a, u64 b, u64 m) {
    u64 res = 0;
    a %= m;
    while (b > 0) {
        if (b % 2 == 1) res = (res + a) % m;
        a = (a * 2) % m;
        b /= 2;
    }
    return res;
}

u64 power_mod(u64 base, u64 exp, u64 mod) {
    u64 res = 1;
    base %= mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = mul_mod(res, base, mod);
        base = mul_mod(base, base, mod);
        exp /= 2;
    }
    return res;
}

bool is_prime_miller_rabin(u64 n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;
    u64 d = n - 1;
    int s = 0;
    while (d % 2 == 0) { d /= 2; s++; }
    static const u64 bases[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23 };
    for (u64 a : bases) {
        if (n <= a) break;
        u64 x = power_mod(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool composite = true;
        for (int r = 1; r < s; r++) {
            x = mul_mod(x, x, n);
            if (x == n - 1) { composite = false; break; }
        }
        if (composite) return false;
    }
    return true;
}

// --- 2. STRUTTURE E LOGICA GC-60 ---
struct punti : std::numpunct<char> {
    char do_thousands_sep()   const override { return '.'; }
    std::string do_grouping() const override { return "\3"; }
};

const u64 MODULO = 60;
const u64 CERCA_IN = 2000000000ULL;
const u64 N_SOTTOLISTE = (CERCA_IN / MODULO) + 1;
const int RESIDUI[] = { 1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57 };
const u64 SEG_SIZE = 32768;

int residuo_to_bit[60];

struct ColpoResiduo {
    u64 prossima_L;
    uint16_t maschera_NOT;
};

struct PrimoAtleta {
    u64 p;
    std::vector<ColpoResiduo> colpi;
};

bool is_prime_simple(u64 n) {
    if (n < 2) return false;
    if (n < 4) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    for (u64 i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0) return false;
    return true;
}

int main() {
    std::cout.imbue(std::locale(std::cout.getloc(), new punti));
    for (int i = 0; i < 60; i++) residuo_to_bit[i] = -1;
    for (int i = 0; i < 16; i++) residuo_to_bit[RESIDUI[i]] = i;

    std::cout << "--- Sieve Wheel M60 ricarca su: " << CERCA_IN <<" (AMD Ryzen 7 4800U) ---" << std::endl;

    std::vector<uint16_t> lista_completa;
    try {
        lista_completa.resize(N_SOTTOLISTE, 0xFFFF);
    }
    catch (...) {
        std::cerr << "Errore: RAM insufficiente per 100 mld." << std::endl;
        return 1;
    }

    u64 limite = (u64)std::sqrt(CERCA_IN + 60);
    std::vector<PrimoAtleta> database;

    for (u64 p = 7; p <= limite; p++) {
        if (!is_prime_simple(p)) continue;
        PrimoAtleta pa;
        pa.p = p;
        u64 n = p * p;
        int p_num = (int)((n - 10) % 60); // FIX: Dichiarato qui
        u64 p_lista = (n - 10) / 60;
        int delta = (2 * p) % 60;
        u64 carry_base = (2 * p) / 60;
        int n_init = p_num;
        bool first = true;
        uint16_t maschere_temp[60] = { 0 };
        u64 prime_pos_temp[60];
        std::vector<int> residui_visti;
        while (first || p_num != n_init) {
            first = false;
            int b_idx = residuo_to_bit[p_num];
            if (b_idx != -1) {
                if (maschere_temp[p_num] == 0) {
                    residui_visti.push_back(p_num);
                    prime_pos_temp[p_num] = p_lista;
                }
                maschere_temp[p_num] |= (1 << b_idx);
            }
            int temp = p_num + delta;
            p_lista += carry_base + (temp / 60);
            p_num = temp % 60;
        }
        for (int r : residui_visti) pa.colpi.push_back({ prime_pos_temp[r], (uint16_t)~maschere_temp[r] });
        database.push_back(pa);
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    for (u64 s_start = 0; s_start < N_SOTTOLISTE; s_start += SEG_SIZE) {
        u64 s_end = std::min(s_start + SEG_SIZE, N_SOTTOLISTE);
        uint16_t* __restrict ptr = lista_completa.data();
        for (auto& pa : database) {
            u64 p = pa.p;
            for (auto& colpo : pa.colpi) {
                u64 L = colpo.prossima_L;
                uint16_t m = colpo.maschera_NOT;
                while (L < s_end) {
                    ptr[L] &= m;
                    L += p;
                }
                colpo.prossima_L = L;
            }
        }
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    std::cout << "Tempo eliminazione: " << std::chrono::duration<double>(t1 - t0).count() << " secondi" << std::endl;

    // --- FASE 3: VALIDAZIONE ---
    std::cout << "\n--- AVVIO CONTROLLI STRUTTURALI ---" << std::endl;

    u64 conteggio = 3;
    for (u64 i = 0; i < N_SOTTOLISTE; i++) {
        uint16_t mask = lista_completa[i];
        if (mask == 0) continue;
        for (int b = 0; b < 16; b++) if (mask & (1 << b)) conteggio++;
    }
    std::cout << "1. Test Densita': " << conteggio << " candidati rimasti." << std::endl;

    int fallimenti = 0;
    int stampati = 0;
    std::cout << "2. Campione Primi identificati:" << std::endl;
    for (u64 i = 1000; i < 1100; i++) {
        uint16_t mask = lista_completa[i];
        u64 base = i * 60 + 10;
        for (int b = 0; b < 16; b++) {
            if (mask & (1 << b)) {
                u64 n = base + RESIDUI[b];
                if (stampati < 5) { std::cout << "   > " << n << std::endl; stampati++; }
                for (u64 p : {7, 11, 13, 17, 19}) if (n > p && n % p == 0) fallimenti++;
            }
        }
    }
    std::cout << "3. Test Divisibilita': " << (fallimenti == 0 ? "OK" : "FALLITO") << std::endl;

    // --- FASE 4: MILLER-RABIN (Campionamento Casuale) ---
    std::cout << "\n--- CERTIFICAZIONE MILLER-RABIN (100 Test) ---" << std::endl;
    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_int_distribution<u64> dist(0, N_SOTTOLISTE - 1);

    int test_eseguiti = 0, test_ok = 0;
    while (test_eseguiti < 100) {
        u64 idx = dist(rng);
        uint16_t mask = lista_completa[idx];
        if (mask == 0) continue;
        u64 base = idx * 60 + 10;
        for (int b = 0; b < 16; b++) {
            if (mask & (1 << b)) {
                u64 n = base + RESIDUI[b];
                if (is_prime_miller_rabin(n)) test_ok++;
                test_eseguiti++;
                if (test_eseguiti >= 100) break;
            }
        }
    }
    std::cout << "4. Risultato Miller-Rabin: " << test_ok << "/100 OK." << std::endl;

    std::cout << "\nPremi Invio per chiudere...";
    std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');
    std::cin.get();
    return 0;
}