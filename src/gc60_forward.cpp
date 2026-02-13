/*
    GC-60 Forward
    List-Based Structural Sieve (Experimental)

    Version: 0.1
    Status: Experimental
    Author: [Govi Claudio]
    Date: 2026-02-12

    Description:
    Structural extension of the GC-60 prime sieve.
    Replaces runtime multiple generation with modular bitmask propagation.

    Each 60-number block is compressed into 16 residue classes.
    Composite elimination is performed by periodic application
    of precomputed modular masks rather than repeated arithmetic propagation.

    This implementation is written in C++ to evaluate structural
    efficiency with minimal abstraction overhead.

    No claim of asymptotic breakthrough.
    No claim of replacing industrial prime sieves.
*/

// 1. QUESTA RIGA DEVE ESSERE LA PRIMA IN ASSOLUTO per i sistemi windows con IDE Visualstudio.net
#define _CRT_SECURE_NO_WARNINGS 

#include <iostream>
#include <vector>
#include <string>
#include <ctime>
#include <iomanip>
#include <map>
#include <chrono>
#include <algorithm>
#include <random>
#include <cmath>

// Usa unsigned long long per i grandi numeri
typedef unsigned long long u64;

// ----------------------------
// Configurazione
// ----------------------------
const u64 CERCA_IN = 1000000000ULL; // Testato fino a 100 Miliardi
const int RESIDUI[] = { 1, 3, 7, 9, 13, 19, 21, 27, 31, 33, 37, 39, 43, 49, 51, 57 };
const int NUM_RESIDUI = 16;

// Variabili Globali
int residuo_to_bit[60];
std::vector<uint16_t> lista;
u64 n_sottoliste;

std::string get_time_string() {
    std::time_t now = std::time(nullptr);
    std::tm* local_tm = std::localtime(&now);
    char buffer[80];
    if (local_tm) strftime(buffer, sizeof(buffer), "%H:%M:%S", local_tm);
    else return "TimeError";
    return std::string(buffer);
}

void init_mappe() {
    for (int i = 0; i < 60; i++) residuo_to_bit[i] = -1;
    for (int i = 0; i < NUM_RESIDUI; i++) {
        residuo_to_bit[RESIDUI[i]] = i;
    }
}

// ----------------------------
// Logica del Ciclo (GC-60) - ULTRA OTTIMIZZATA CON AVX2 E PUNTATORI
// ----------------------------
void elimina_primo(u64 p) {

    // Vettore statico per riciclare la memoria (evita allocazioni continue)
    static std::vector<std::pair<u64, uint16_t>> maschere_veloci;
    maschere_veloci.clear();

    // --- FASE 1: Calcolo Maschere (Logica originale) ---
    u64 start = p * p;
    u64 temp_div = (start - 10);
    int p_numero = temp_div % 60;
    u64 p_lista_corrente = temp_div / 60;

    int delta = (2 * p) % 60;
    u64 carry_base = (2 * p) / 60;

    int numero_iniziale = p_numero;
    bool primo_giro = true;

    u64 last_saved_lista = p_lista_corrente;
    uint16_t current_mask = 0;
    bool mask_is_dirty = false;

    while (true) {
        if (!primo_giro && p_numero == numero_iniziale) break;
        primo_giro = false;

        if (p_lista_corrente != last_saved_lista) {
            if (mask_is_dirty) {
                maschere_veloci.push_back({ last_saved_lista, current_mask });
            }
            last_saved_lista = p_lista_corrente;
            current_mask = 0;
            mask_is_dirty = false;
        }

        int bit_idx = residuo_to_bit[p_numero];
        if (bit_idx != -1) {
            current_mask |= (1 << bit_idx);
            mask_is_dirty = true;
        }

        int temp = p_numero + delta;
        u64 carry = temp / 60;
        p_numero = temp % 60;
        p_lista_corrente += carry_base + carry;
    }

    if (mask_is_dirty) {
        maschere_veloci.push_back({ last_saved_lista, current_mask });
    }

    // --- FASE 2: Applicazione Maschere con PUNTATORI (Turbo) ---
    // Usiamo il puntatore grezzo per evitare i controlli di sicurezza del vector
    uint16_t* raw_ptr = lista.data();
    u64 limit = n_sottoliste;

    for (const auto& item : maschere_veloci) {
        u64 i = item.first;
        uint16_t mask = ~item.second; // Pre-calcoliamo il NOT della maschera

        // Loop critico: qui AVX2 farà la magia
        while (i < limit) {
            raw_ptr[i] &= mask;
            i += p;
        }
    }
}

void verifica_multipli(u64 p, int campioni = 20) {
    std::cout << "\n--- Verifica mirata multipli ---" << std::endl;
    u64 n = p * p;
    int controllati = 0;

    while (controllati < campioni) {
        u64 R = (n - 10) / 60;
        if (R >= n_sottoliste) break;
        int r = (n - 10) % 60;

        if (residuo_to_bit[r] != -1) {
            int bit = residuo_to_bit[r];
            uint16_t mask = lista[R];
            if (mask & (1 << bit)) {
                std::cout << "ERRORE: " << n << " non e' stato eliminato" << std::endl;
            }
            else {
                std::cout << "OK: " << n << " correttamente eliminato" << std::endl;
            }
            controllati++;
        }
        n += 2 * p;
    }
}

int main() {
    init_mappe();

#ifdef _DEBUG
    std::cout << "!!! ATTENZIONE: SEI IN DEBUG (LENTO) !!! CAMBIA IN RELEASE." << std::endl;
#endif

    std::cout << "Tempo creazione liste iniziale: " << get_time_string() << std::endl;

    u64 calcolo_sottoliste = (CERCA_IN / 60) * 60 + 60;
    n_sottoliste = calcolo_sottoliste / 60;
    if (calcolo_sottoliste < CERCA_IN) n_sottoliste += 1;

    std::cout << "Numero sottoliste: " << n_sottoliste << std::endl;
    std::cout << "Copertura fino a: " << n_sottoliste * 60 << std::endl;

    try {
        lista.resize(n_sottoliste, 0xFFFF);
    }
    catch (const std::bad_alloc& e) {
        std::cerr << "Errore fatale: Memoria insufficiente! " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Liste create: " << lista.size() << std::endl;
    std::cout << "Tempo creazione liste finale: " << get_time_string() << std::endl;

    // ESECUZIONE
    auto t0 = std::chrono::high_resolution_clock::now();

    // 1. Manuale p=7
    u64 p_start = 7;
    elimina_primo(p_start); // Scommenta se vuoi fare il 7 separatamente, ma il ciclo sotto lo gestisce

    // 2. Limite
    u64 max_copertura = n_sottoliste * 60 + 60;
    u64 limite = (u64)std::sqrt(max_copertura);

    std::cout << "Avvio scansione automatica fino a: " << limite << std::endl;

    // 3. Ciclo
    for (u64 R = 0; R < n_sottoliste; R++) {
        u64 base = R * 60 + 10;
        if (base > limite) break;

        uint16_t mask = lista[R];
        if (mask == 0) continue;

        for (int bit = 0; bit < 16; bit++) {
            if (mask & (1 << bit)) {
                u64 p_corrente = base + RESIDUI[bit];

                if (p_corrente > limite) break;

                // Chiama la funzione di eliminazione
                elimina_primo(p_corrente);
            }
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = t1 - t0;

    std::cout << "Scansione completata." << std::endl;
    std::cout << "Tempo Totale (Scansione fino a " << limite << "): " << diff.count() << " secondi" << std::endl;

    // Verifica personalizzata
    char risposta;
    std::cout << "\nVuoi verificare la presenza di multipli di un numero specifico? (s/n): ";
    std::cin >> risposta;

    if (risposta == 's' || risposta == 'S') {
        u64 P;
        std::cout << "Inserisci il numero primo (divisore) da testare: ";
        std::cin >> P;

        if (P == 0) {
            std::cout << "Errore: il divisore non può essere zero." << std::endl;
        }
        else {
            std::cout << "\n--- Controllo casuale a campione per il divisore: " << P << " ---" << std::endl;

            // Generatore di numeri casuali per selezionare le sottoliste
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<u64> dist(0, n_sottoliste - 1);

            // Test su 5 liste estratte a sorte
            for (int i = 0; i < 5; i++) {
                u64 R = dist(gen);
                uint16_t mask = lista[R];
                u64 base = R * 60 + 10;

                std::cout << "\nAnalisi Lista " << R << " (Base " << base << ")" << std::endl;
                bool trovato_errore = false;

                for (int bit = 0; bit < 16; bit++) {
                    if (mask & (1 << bit)) {
                        u64 numero = base + RESIDUI[bit];

                        // Verifica se il candidato è un multiplo di P (e non è P stesso)
                        if (numero > P && numero % P == 0) {
                            std::cout << "  >> ERRORE: " << numero << " e' multiplo di " << P << std::endl;
                            trovato_errore = true;
                        }
                    }
                }

                if (!trovato_errore) {
                    std::cout << "  >> OK: Nessun multiplo di " << P << " trovato." << std::endl;
                }
            }
        }
    }
    return 0;
}
