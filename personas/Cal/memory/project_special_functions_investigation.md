---
name: Special functions investigation plan
description: Casey directive May 1 — investigate ALL generalized special function representations of BST spectral zeta; Fox H partially done, Aleph/Heckman-Opdam/Nahm/mock theta all NEW
type: project
---

## Casey Directive (May 1, 2026)

"I actually want to pursue them all. Make all investigations a go."

## Status of Each Investigation

### DONE (have toys, need review for spectral zeta application)
- **Meijer G**: Toy 1301 (BST framework), Toy 1316 (RH via Meijer G). Established that BST functions ARE Meijer G. Need: apply to ζ_B(s) specifically.
- **Fox H**: Toy 1302 (depth reduction). Showed Fox H with BST-rational multipliers reduces to Meijer G. BUT: this was about COMPOSITION depth, not about REPRESENTING ζ_B(s). **NEEDS NEW INVESTIGATION** — can ζ_B(s) itself be written as a Fox H function?
- **Selberg zeta**: 2 toys (473, 1196). Selberg trace formula explored. Paper #86 uses it. Need: connect Selberg PRODUCT formula to ζ_B specifically.
- **Automorphic forms**: 140+ file references. Extensive work on Shimura varieties, Hecke operators. Need: focus specifically on automorphic forms on SO(5,2).
- **Siegel modular**: 10 files. Some computational work. Need: test if Θ(t) is a Siegel modular form of degree 2.
- **Maass forms**: 7 files including Toy 326. Need: connect to ζ_B's eigenfunction expansion.

### NEW (no existing work)
- **Aleph (ℵ) function**: NO work. TOP PRIORITY. Non-integer Gamma shifts match our half-integer root structure.
- **Heckman-Opdam for B_2**: NO work. PURPOSE-BUILT for our situation. Root system B_2 with multiplicities (3,1). A specialist would recognize ζ_B immediately.
- **Mock theta functions**: NO work. Lyra's Phi(3)=-1 (antisymmetric) is reminiscent of mock theta shadow corrections.
- **Nahm sums**: NO work. q-series with modular properties from Cartan matrix. Our Hilbert function could define a Nahm-type sum.

## Priority Order (Casey approved all)

1. **Fox H representation of ζ_B(s)** — partially done, needs specific application
2. **Aleph function** — most likely to capture full polynomial weight structure
3. **Heckman-Opdam for B_2 with (p,q)=(3,1)** — purpose-built, may give FE immediately
4. **Selberg zeta product** — connects eigenvalues to geodesics (mass = winding)
5. **Siegel modular** — if Θ(t) is Siegel degree 2, FE follows from Siegel modular group
6. **Mock theta** — if Phi correction IS the mock shadow
7. **Nahm sums** — B_2 Cartan matrix defines the Nahm sum
8. **Automorphic on SO(5,2)** — nuclear option, guaranteed but heavy

## Key Question

Can ζ_B(s) be written as:
- Fox H: H_{p,q}^{m,n}(z | parameters from BST) ?
- Aleph: ℵ_{p,q}^{m,n}(z | α_j from root structure) ?
- Heckman-Opdam: F_λ(a; k) with a = B_2, k = (3,1) ?

If ANY of these gives a clean representation, the functional equation follows from KNOWN theory. The Gamma factor hunt ends with a citation, not a proof.

## What to check first when tokens renew
1. Read Toy 1301 and 1302 — verify what was established about Meijer G/Fox H
2. Test: write ζ_B(s) = Fox H with parameters {rank, N_c, n_C, C_2, g}
3. If Fox H works → extract FE from Fox H theory
4. If not → try Aleph with half-integer α_j from root shifts 1/2, 3/2
5. If neither → Heckman-Opdam is the guaranteed path for B_2
