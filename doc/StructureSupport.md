# Data-Structure Support:  Planning Document
Time-stamp: "2013 Dec 21 14:32:58 PST"

## Background and Motivation

The abstract interface currently provided by `Module` and `ModuleInput`, in which input values
are restricted to scalar data-types, is less than ideal.  It works, but any module requiring
simultaneous updates to multiple scalar-valued inputs is at the mercy of the connection's master
implementation since we provide no means of declaring such a requirement.  But implementing a
system for such requirements would be messy and complicated, and luckily there's a better way.

Providing support for data structures as input data-types would allow a more natural approach to
the problem; instead of declaring three separate inputs that require simultaneous updates, a
module would simply declare a single input taking a structure that contains a field for each of
the previously-declared inputs.

## Considerations and Goals

We already have several requirements:

  * Module inputs should support structured data types
  * `ModuleControl` instances should support setting, wrapping, and fetching those structured
    data types.

Support for arbitrary data _structures_ adds yet another layer of complexity to the problem of
making a device interface easily-understandable.  In the case of this project in particular,
we'd like to ensure that the final interface remains discoverable and self-documenting without
access to the device's source code; this means we'll need to be able to describe (and perhaps
generate definitions of) these data structures independently of that source code.  And since a
data structure is comparatively more opaque than a single scalar value, it's not unreasonable to
demand that all data-structure representations come with built-in documentation.

  * Any structure type used as a module input must have an accompanying transmissible
    representation that may be used to reconstruct that structure-type's definition.
    - These representations should include documentation that clearly explains the semantic
      intent of the type, as well as that of all member fields.


## Related Existent Code

We currently support scalar-valued data declarations via the `DataDeclaration<...>` classes, and
there is basic support for documentation-*mandatory* structured data in the `DataStructure`
class (via `APIElement`).  `DataStructure` uses `DataDeclaration` (via `DataField`) to represent
its fields, so any enhancements to `DataDeclaration` will automatically benefit `DataStructure`
itself.

However, the `DataDeclaration` classes are currently a complete mess, with data-value constraints
mixed in with the data-type specifications, and is otherwise generally disorganized and
undocumented.  Constraints apply to fields, not individual types, and this (along with other
cleanups) should be reflected in the code.  The effort to provide a clean, well-planned
structure-type API would be much benefited by a redesign of the data-declaration classes, and will
be postponed until that rewrite is complete.


## Conclusion

Sub-project on hold (see previous section).  This document will be updated when status changes.
