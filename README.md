# Bonk

_Jason Klein, Elmo Tang, Noah Hursh_

Group project for CS 4621: Computer Graphics Practicum.

Work in progress.

## Development

### Visualization

The visualizer is a React app located at `viz/`. Run the dev server with

```bash
# Must be run from inside viz/
npm run dev
```

and open http://localhost:3000 on your computer.

## Using Docker

This project uses Docker to streamline cross-platform development, which is especially useful when working with libraries like [CGAL](https://www.cgal.org/) that would otherwise have different, system-level installs for MacOS and Windows.

### Devcontainer

This project is meant to be used with [devcontainers](https://containers.dev/) for development. This will configure your IDE (VSCode or several others) to create a special Docker image based on the Dockerfile and do all development *inside that docker container*. 

To set up the devcontainer in VSCode, run the `Dev Containers: Open Folder in Dev Container...` command from the command palette and select the project folder. It will take a bit to load the first time because it's doing all the installation.

### Manual Usage

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
