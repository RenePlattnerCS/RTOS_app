from invoke import task

# name of the docker image
project_name = "cproject"

@task
def builder_build(c):
    """
    Build the Docker builder image
    """
    c.run(f"docker build -f builder.Dockerfile -t {project_name}-builder:latest .")

@task
def builder_run(c):
    """
    Run an interactive shell in the Docker builder container
    """
    c.run(
        f"docker run --rm -it "
        f"--platform linux/amd64 "
        f"--workdir /builder/mnt "
        f"-v $(pwd):/builder/mnt "
        f"{project_name}-builder:latest /bin/bash",
        pty=True
    )
