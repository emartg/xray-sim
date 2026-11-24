#!/usr/bin/env python3
"""Check and render the diagrams of docs/diagrams/.

The same script as the one of the Engine (opengl-graphics-engine, docs/diagrams/diagrams.py at e363d5e), copied unchanged except for
this paragraph, so that the Engine, gVirtualXRay, and the simulator check and render their diagrams in the same way.

Every diagram is a pair in a subfolder: <name>.mmd (the Mermaid source, the file to edit) and <name>.png (the rendered image).
The images that do not have a source yet are listed in pending_sources.txt, and the list can only shrink.

    python docs/diagrams/diagrams.py check    every .png has its .mmd and every .mmd has its .png (no tools needed; used by the CI)
    python docs/diagrams/diagrams.py render   regenerate every .png with the pinned Mermaid command-line tool (needs Node.js)

The environment variable DIAGRAMS_MMDC names an installed mmdc program to use instead of npx, and DIAGRAMS_PUPPETEER_CONFIG names
a Puppeteer configuration file (for example to choose the browser).
"""

import os
import shutil
import subprocess
import sys

MERMAID_CLI_VERSION = "11.17.0"
SCALE = "2"
BACKGROUND = "white"
PNG_SIGNATURE = b"\x89PNG\r\n\x1a\n"

ROOT = os.path.dirname(os.path.abspath(__file__))
PENDING_FILE = os.path.join(ROOT, "pending_sources.txt")


def find_files(extension):
	"""The paths, relative to the diagrams folder, of the files with the extension that are in a subfolder."""
	found = set()
	for folder, _, names in os.walk(ROOT):
		if os.path.abspath(folder) == ROOT:
			continue
		for name in names:
			if name.endswith(extension):
				path = os.path.relpath(os.path.join(folder, name), ROOT)
				found.add(path[: -len(extension)].replace(os.sep, "/"))
	return found


def read_pending():
	"""The images (without the extension) that are listed in pending_sources.txt: one path per line, blank lines and # comments ignored."""
	pending = set()
	if not os.path.exists(PENDING_FILE):
		return pending
	with open(PENDING_FILE, encoding="utf-8") as file:
		for line in file:
			line = line.strip()
			if line and not line.startswith("#"):
				pending.add(line[: -len(".png")] if line.endswith(".png") else line)
	return pending


def check():
	sources = find_files(".mmd")
	images = find_files(".png")
	pending = read_pending()
	problems = []
	for name in sorted(sources - images):
		problems.append("%s.mmd has no %s.png" % (name, name))
	for name in sorted(images - sources - pending):
		problems.append("%s.png has no %s.mmd (draw the source, or list the image in pending_sources.txt)" % (name, name))
	for name in sorted(pending & sources):
		problems.append("%s.png is listed in pending_sources.txt but has its %s.mmd (remove the line)" % (name, name))
	for name in sorted(pending - images):
		problems.append("%s.png is listed in pending_sources.txt but does not exist (remove the line)" % name)
	for name in sorted(sources & images):
		with open(os.path.join(ROOT, name + ".png"), "rb") as image:
			if image.read(len(PNG_SIGNATURE)) != PNG_SIGNATURE:
				problems.append("%s.png is not a PNG image" % name)
		if not os.path.getsize(os.path.join(ROOT, name + ".mmd")):
			problems.append("%s.mmd is empty" % name)
	for problem in problems:
		print("[ERROR::DIAGRAMS::check] " + problem)
	if problems:
		return 1
	print("[SUCCESS::DIAGRAMS::check] %d diagrams, each with its .mmd and its .png; %d images wait for their source" %
		(len(sources), len(pending & images)))
	return 0


def render():
	mmdc = os.environ.get("DIAGRAMS_MMDC")
	if mmdc:
		command = [mmdc]
	else:
		npx = shutil.which("npx")
		if not npx:
			print("[ERROR::DIAGRAMS::render] npx was not found, install Node.js or set DIAGRAMS_MMDC")
			return 1
		command = [npx, "--yes", "@mermaid-js/mermaid-cli@" + MERMAID_CLI_VERSION]
	config = os.environ.get("DIAGRAMS_PUPPETEER_CONFIG")
	sources = sorted(find_files(".mmd"))
	for name in sources:
		arguments = command + ["-i", os.path.join(ROOT, name + ".mmd"), "-o", os.path.join(ROOT, name + ".png"),
			"-b", BACKGROUND, "-s", SCALE]
		if config:
			arguments += ["-p", config]
		print("[INFO::DIAGRAMS::render] Rendering " + name)
		if subprocess.call(arguments) != 0:
			print("[ERROR::DIAGRAMS::render] " + name + ".mmd was not rendered")
			return 1
	print("[SUCCESS::DIAGRAMS::render] Rendered %d diagrams with Mermaid CLI %s" % (len(sources), MERMAID_CLI_VERSION))
	return check()


if __name__ == "__main__":
	if len(sys.argv) != 2 or sys.argv[1] not in ("check", "render"):
		print(__doc__)
		sys.exit(2)
	sys.exit(check() if sys.argv[1] == "check" else render())
