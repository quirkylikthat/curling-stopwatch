# PlatformIO extra script for the [env:native] environment.
#
# Adds a "Run unit tests" task (VS Code: PlatformIO sidebar > native > Custom)
# that builds the host test program and runs it. A failing test makes the task
# fail. CLI equivalent:  pio run -e native -t tests
Import("env")  # noqa: F821  (provided by PlatformIO/SCons)

env.AddCustomTarget(  # noqa: F821
    name="tests",
    dependencies="$PROGPATH",
    actions='"$PROGPATH"',
    title="Run unit tests",
    description="Build and run the host unit tests (src/core + ViewBuilder)",
    always_build=True,
)
