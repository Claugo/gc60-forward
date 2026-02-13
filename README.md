# GC-60: Structural Sieve Extension (Experimental)

---

## Overview

This project represents an experimental evolution of the GC-60 prime generation method.
While the standard GC-60 operates via modular reduction (mod 60), segmented storage, and arithmetic elimination of multiples, this branch introduces a different operational model: Precomputed Structural Masks.

Note: This implementation does not replace **MicroPrime**. It is an exploration of a different representation of composite elimination.

---

## Context: GC-60 Recap

GC-60 is based on:

- Residue classes coprime with 60 (16 valid residues)
- Segmented memory layout
- Deterministic elimination through modular stepping
- Arithmetic propagation of multiples

The classical elimination model can be summarized as:

```cpp
for each prime p:
    generate multiples of p
    mark composite positions
```    
Even under heavy optimization, the core remains arithmetic.

Multiples are generated.  
Positions are computed.  
Composites are marked.


## Structural Model

This implementation changes how elimination information is encoded.

Instead of generating multiples during execution:

1.  The modular trajectory of a prime is computed once.
    
2.  That trajectory is converted into bit masks.
    
3.  Masks are applied periodically across memory blocks.
    

Elimination becomes structural.

The arithmetic phase is limited to mask construction.  
After that, elimination is reduced to bitwise operations and periodic block stepping.
```cpp
`block[i] &= ~mask; i += p;` 
```
No per-candidate division.  
No runtime modulo.  
No repeated multiple testing.

----------
## Representation

Each memory block encodes 60 integers using only the 16 residue classes coprime with 60.

Each block is a 16-bit word.

Block R represents:
[60R + 10, 60R + 69]

Each bit represents one residue class.

Memory stores structural state, not integers.

## Conceptual Difference

This is not a multiplication-to-addition optimization.

It is a representational shift.

Classical sieve:

-   Information is generated procedurally
    Structural sieve:

-   Information is encoded once as a modular configuration.
    
-   Elimination is the application of that configuration.
    

Multiples are no longer runtime events.  
They become structural properties embedded in masks.

A composite is not tested.  
It belongs to a preclassified modular trajectory.

----------

## Status

-   Empirically validated
    
-   Stable architecture
    
-   AVX2-friendly
    
-   Focused on representation efficiency
    

## Implementation Note

This experimental model is implemented in C++.

The choice of C++ is deliberate: it provides direct control over memory layout, bitwise operations, and pointer-level manipulation.

The objective is not language preference, but measurement accuracy.  
The structural model is evaluated with minimal abstraction overhead, ensuring that performance observations reflect representation efficiency rather than runtime framework effects.

No claim of asymptotic breakthrough.  
No claim of replacing industrial prime sieves.

## Build

Compile with:
g++ -O3 -std=c++17 src/gc60_forward.cpp -o gc60_forward
