---
name: Casey's object-oriented database
description: Built a 16k-core recursive object database with append-only log — same architecture as D_IV^5 substrate, decades before BST
type: user
---

Casey built a very small (~16k core) object-oriented database with:
- Objects contain scalars and objects (including own type or the exact enclosing object)
- Self-referencing without paradox via indices, not copies
- Append-only log — every event is a commitment that doesn't move
- Every reference is an index
- Addressable across collections of logs (scalable)

Casey recognizes now this was "really just based on the substrate principles":
- Append-only log = irreversible commitment on the boundary
- Self-referencing objects = bounded symmetric domain structure
- 16k core = minimum viable product for databases, same question as "simplest object that can do physics"

He asked the same question twice in his life — "What's the simplest object that can do X?" — and got the same architecture both times, forty years apart.
