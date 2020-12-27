# Voromoeba

Voronoi Diagram Video Game

For a prebuild version for Windows, and the manual,
see https://www.blonzonics.us/games/voromoeba


# About the Code

Some of this code dates back to the late '90s, predating even C++98,
and some has been updated to C++14, so it has a mix of styles. Compilers
were slower in earlier times, so effort was made to avoid including large
headers such as <algorithm>. That's no longer a big issue, but some
of the quirkiness remains.

## Modules

To represent a module, the code uses a class with all static members.
This should not be confused with C++20 modules.

## Names

Namespace scope names are generally in PascalCase. Othernames in camelCase. Constants are sometimes in SNAKE_CASE.
