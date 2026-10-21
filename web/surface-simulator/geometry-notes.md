# Hardware geometry

## Scope and evidence

`surfaces.js` is original authored geometry, preserving the verified manufacturer plan footprints and all 146 original control IDs. It is an interactive orthographic, axonometric hardware representation, not dimensionally certified CAD. It contains no raster images or third-party scripts. Firmware/state integrations remain separate.

The hardware assembly consists of two case shells, 37 piano keys, 16 raised pads, 16 rotary encoders with bases and pointers, 12 fader rails/caps, two ribbed wheels, screen bezel/glass, 18 Elite utility buttons and 45 SMC buttons. Actual printed hardware labels are retained because this is a hardware replica, not an abstract explanatory diagram.

Plan units retain the earlier 1000-wide reference: Elite1000×395; SMC1000×477. Heights are illustrative: case top30, pads36, natural key top35, accidental top52, Elite encoder top63, SMC encoder top78. They are not manufacturer measurements.

## Rendering method

An explicit 3D-to-2D orthographic camera uses azimuth35° and elevation40°. Every visible polygon vertex is projected from `(x,y,z)`. `fitProjection` determines scale, translation and viewBox from envelope corners including raised components. This is not a flat CSS/SVG skew. Projected viewBoxes are1000×640 (Elite) and1000×652 (SMC).

Each rectangular solid has visible vertical faces, a narrow bevel ring and a lit top. Encoders are tapered16-sided solids, with shade chosen from four-tone palettes. Case bottom seam, upper wall and top are distinct solids. Repeated control assemblies are sorted by camera depth; individual cylinder faces are sorted separately; natural-key bed is drawn before raised accidentals. Slots and printed legends remain on the case plane. This is a bounded painter's construction, not a general 3D hidden-surface engine or third-party collision audit.

Read-only design references consulted: pinned Anatomy `SKILL.md` and `references/craft.md` at commit1482e8469b0a9e0d8cc18ea2740a01423d29ae80. Example URL could not be fetched. No Anatomy code imported or executed. Manufacturer anatomy references remain in REFERENCES.md.

## Public API

Exports remain `renderSmk()`, `renderSmc()`, `surfaceGeometry`. A pure `fitProjection(points,width=1000,pad=24)` is also exported.

For `g=surfaceGeometry.smk` or `.smc`:

- `g.width`, `g.height`: projected SVG viewBox dimensions.
- `g.physicalWidth`, `g.physicalHeight`: original plan dimensions.
- `g.project(x,y,z=0) -> {x,y}`: world coordinates to SVG viewBox coordinates.
- `g.unproject(svgX,svgY,z=0) -> {x,y}`: inverse on a known height plane. Pass case-top30 for general panel picking; inverse is not a ray/solid intersection.
- `g.planeTransform(z)`: SVG affine matrix for text or other2D content using original plan coordinates.
- `g.knobTransform(index,degrees)`: full affine transform for `[data-pointer]`, accepting unrestricted relative encoder angle in degrees. Apply using `setAttribute('transform',...)`; do not add a rotation in already-projected coordinates.
- `g.faderTransform(index,value)`: SVG translation for `[data-cap]`, accepting0–127 with clamping; entire three-dimensional cap and stripe move together. Fader0 is physical lower stop,127 upper stop. Do not set a `y` attribute on the group.
- `g.faderValue(index,svgX,svgY)`: inverse picking on cap-top plane, returning clamped0–127. Input must first be converted from client coordinates through the SVG's inverse screen CTM.
- `g.keyTransform(pressed)`: downward projected displacement for key press illustration. False restores zero translation.
- `.knobs[]`, `.faders[]`: plan coordinates and dimensions; faders retain `.defaultCapY` for compatibility.
- `#smk-screen`: affine group at z33.2, keeping LCD bounds x143,y27,width67,height65. Populate using original LCD coordinates. Do not replace its transform.

`[data-face]` is a polygon on the actual lit top of a key, pad or button. Set its fill for live feedback; do not use a rectangle-only selector. `[data-pointer]` and `[data-cap]` are groups, not individual paths/rectangles. Every control remains a focusable group with accessible label and title.

## Validation

- `node tests/anatomy.mjs`:146 unique IDs, exact model counts, no embedded images.
- `node tests/geometry.mjs`: projected polygon bounds, finite coordinates, project/unproject round trips across multiple heights, complete fader travel/inverse picking, degree pointer API, group counts, visible side faces and affine screen.
- Generated static SVGs and1600px PNGs in `evidence/geometry`; both PNGs inspected using Inkscape rasterization and image viewing. Clear raised parts, preserved footprints, no clipping at viewBox. Interactive integration belongs to parent; these checks do not claim firmware execution or hardware validation.
