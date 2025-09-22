# Bonk

_Jason Klein, Elmo Tang, Noah Hursh_

Group project for CS 4621: Computer Graphics Practicum.

Work in progress.

## Using Docker

This project uses Docker to streamline cross-platform development, which is especially useful when working with libraries like [CGAL](https://www.cgal.org/) that would otherwise have different, system-level installs for MacOS and Windows.

Build the Docker image with

```bash
# Run from the project root
docker build -t bonk .
```

Run the default command with

```bash
docker run --rm bonk
```

For an interactive terminal, run

```bash
docker run -it --rm bonk bash
```

Note that the `--rm` flag will delete the container after the command finishes! You should use [bind mounts](https://docs.docker.com/engine/storage/bind-mounts/) to persist data across commands if not using a devcontainer (devcontainers will do their own thing to persist data).
