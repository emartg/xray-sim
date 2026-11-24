# Diagrams

Each diagram is kept twice, side by side, in a subfolder named after its topic:

```
docs/diagrams/<subfolder>/<name>.mmd   the Mermaid source (the file to edit)
docs/diagrams/<subfolder>/<name>.png   the rendered image (generated from the source)
```

The images are regenerated and reviewed whenever a release is about to come out, so that every `.png` matches its `.mmd`.

## Index

| Diagram | Source | Image | What it shows |
|---|---|---|---|

No diagram is committed yet: each one gets its line here when it is added. An image that has no source yet would be listed in `pending_sources.txt` (one path per line, relative to this folder), which the check accepts; the simulator has no such image, so the file does not exist.

**The UML diagrams will be a hierarchy**, as the Engine's: an overview of the modules first, and one detailed diagram for each module that the overview shows as a block. Each `.mmd` of the set says in its first lines which module it shows and which block of the overview it details.

## Rendering

The images are made by the Mermaid command-line tool, **`@mermaid-js/mermaid-cli` version 11.17.0** (Mermaid 11.17), which needs Node.js and a Chromium-based browser (the tool downloads one the first time it runs). One command renders every diagram:

```
python docs/diagrams/diagrams.py render
```

It runs this command for each `.mmd` (the version is written in the script, `MERMAID_CLI_VERSION`):

```
npx --yes @mermaid-js/mermaid-cli@11.17.0 -i docs/diagrams/<subfolder>/<name>.mmd -o docs/diagrams/<subfolder>/<name>.png -b white -s 2
```

The environment variable `DIAGRAMS_PUPPETEER_CONFIG` names a Puppeteer configuration file (to choose another browser or to pass flags), and `DIAGRAMS_MMDC` names an installed `mmdc` program to use instead of `npx`. The fonts and the colors are in the source of each `.mmd` (the `config:` block of its front matter and its `classDef` lines), and the position of every box is written in it, so that the source alone produces the image. The size of the text depends on the fonts of the browser, so two renders of the same source can differ in size and layout, not only by a few pixels (in the Engine, a render on another machine was 2400 by 1350 pixels where the committed one is 1568 by 882).

**A render is committed only when it is more accurate or clearer than the image it replaces, and together with the `.mmd` that produced it.** Look at every image before committing it, and check it against the code. A render of a source that did not change, which only looks different, is discarded (`git restore` the image). A source that does not reproduce the committed image is first brought to its quality (a title, a theme, a legend that says what each color is), or the committed image is kept.

## Checking

```
python docs/diagrams/diagrams.py check
```

stops with an error when a `.png` has no `.mmd` and is not listed in `pending_sources.txt`, when a `.mmd` has no `.png` (the image was not rendered), when a listed image already has its `.mmd` or does not exist (the line is removed), when an image is not a PNG file, or when a source is empty. It needs only Python and runs in the workflow `diagrams.yml` on every pull request and push to `main`, in a few seconds. It does not check that an image is up to date with its source: that is what rendering and reviewing before a release is for.

## Adding a diagram

1. Write `docs/diagrams/<subfolder>/<name>.mmd` (names in lowercase with underscores) from the code, not from memory, and add its line to the index above.
2. Render it with `python docs/diagrams/diagrams.py render` and look at the image. Commit only the image of a diagram whose source changed and whose render is more accurate or clearer (see the note on rendering).
3. If the image was in `pending_sources.txt`, remove its line.
4. Commit the `.mmd` and the `.png` together, in a `docs/` commit of its own (never with code), and show the image in the documentation where it belongs.

Choose the Mermaid diagram type for the layout that the diagram needs: a `flowchart` places the boxes by itself, and a `block-beta` diagram places every box in a grid that the source writes (the Engine's dependency diagram uses it: the targets are stacked in the middle and each one has its libraries beside it, never more than one row away, so that no arrow crosses a box and the image stays compact). Arrows go straight between the centers of two boxes, so keep a box off the line between two others. A block diagram has no `linkStyle`, so the kind of an arrow is written as the text on it.

**Class diagrams** (`classDiagram`) are laid out with the ELK engine (`layout: elk` in the front matter), which draws orthogonal arrows around the boxes instead of through them; `class: useMaxWidth: false` keeps the natural size. The image is the diagram scaled to the width of the browser window of the tool (about 784 px, times the scale 2), so a diagram whose natural width is larger than that gets smaller text: read the natural size from the `viewBox` of the SVG (`mmdc -i name.mmd -o name.svg`) and keep it under about 1600 px, which gives text of about 1% of the width of the image, the size of the images that the Engine's UML set replaced. When the layout of a class diagram grows too tall or too wide, remove the members that do not explain the diagram, give a box at least one member so that it has no empty compartment, and try `direction LR` against `direction TB`. A catalogue of classes that are not related to each other (like the Engine's `utilities_and_geometry_data`) cannot be laid out as a class diagram, because the automatic layouts put unrelated classes in one row or one column: draw it as a block diagram whose blocks imitate the class compartments (see below).

**UML diagrams use UML class blocks and keep one style** (the owner's decision for every repository, 2026-11-09): every diagram of `uml/` draws its classes, interfaces, and subsystems as class blocks (the stereotype and the name, then the attributes and the operations, with the arrowheads of UML), with the fonts, the colors, and the stereotypes of the others, and plain boxes (`block-beta`, flowcharts) are for the diagrams that are not UML (dependencies, data flows, sequences, and charts). Legibility and correctness come first, and the size of the image that is replaced is a reference, never a reason to drop the notation or the information. When a class diagram cannot be laid out legibly by the tool, as for a catalogue of unrelated classes, a block diagram can draw the blocks with the compartments of a class: each block is a label of HTML with `<br/>`, `<hr/>` between the compartments, `<div align='left'>` for the members, `&laquo;` and `&raquo;` for the stereotype (the source stays in ASCII), `<u>` for static members, and the fill `#fff4dd` and the border `#eedebb` of the class blocks of Mermaid; its relations that are real are drawn as dotted arrows with a free cell between the blocks they join.
