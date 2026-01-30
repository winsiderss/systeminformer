#!/usr/bin/env python3

import os
import typing as T
from datetime import datetime, timezone
from pathlib import Path
from typing import TYPE_CHECKING


def _strict_int(x: str) -> int:
    if not x.isdecimal() or not x.isascii():
        msg = f"not decimal: {x!r}"
        raise ValueError(msg)
    return int(x)


class VersionInfo(T.NamedTuple):
    major: int = 0
    minor: int = 0
    build: int = 0
    revision: int = 0

    @T.override
    def __str__(self) -> str:
        return ".".join(str(x) for x in self)

    @classmethod
    def size(cls) -> int:
        return len(cls())

    @classmethod
    @T.overload
    def parse(cls, string: str, *, full: T.Literal[False] = False) -> list[int]: ...

    @classmethod
    @T.overload
    def parse(cls, string: str, *, full: T.Literal[True]) -> T.Self: ...

    @classmethod
    def parse(cls, string: str, *, full: bool = False) -> list[int] | T.Self:
        version: list[int] = []
        s = cls.size()

        parts = string.split(".")
        parsed_len = len(parts)
        if parsed_len > s:
            msg = (
                f"Too many components in version number: {string!r}",
                f"Expected {s}, got {parsed_len}",
            )
            raise ValueError(*msg)

        if full and parsed_len != cls.size():
            msg = (
                f"Need a full version number: {string!r}",
                f"Expected {s} components, got {parsed_len}",
            )
            raise ValueError(*msg)

        for part in parts:
            try:
                version.append(_strict_int(part))
            except ValueError as e:
                msg = f"Version component is not decimal: {part!r}"
                raise ValueError(msg).with_traceback(e.__traceback__)

        return cls(*version) if full else version


def _datetime_to_version(time: datetime | None) -> tuple[int, int]:
    time = time.astimezone(timezone.utc) if time else datetime.now(timezone.utc)
    timetuple = time.utctimetuple()
    y = time.year % 100
    j = timetuple.tm_yday
    h = time.hour + 1
    m = time.minute
    return y * 1000 + j, h * 1000 + m


def _calculate_version(path: Path) -> VersionInfo:
    version = list(
        VersionInfo(
            0,
            0,
            *_datetime_to_version(None),
        )
    )

    try:
        from git import InvalidGitRepositoryError, Repo

        try:
            repo = Repo(path, expand_vars=False)

            last = next(repo.iter_commits("--tags", max_count=1))
            for tag in repo.tags:
                if tag.commit == last and tag.name.startswith("v"):
                    version = VersionInfo.parse(tag.name.removeprefix("v"))
                    break

            version[2], version[3] = _datetime_to_version(
                None if repo.is_dirty() else repo.head.commit.committed_datetime
            )
        except InvalidGitRepositoryError:
            pass
    except ImportError:
        pass

    for i, envname in enumerate(("BUILD_MAJORVERSION", "BUILD_MINORVERSION")):
        if env := os.environ.get(envname):
            try:
                version[i] = _strict_int(env)
            except ValueError as e:
                msg = f"Version component is not decimal: {env!r}"
                raise ValueError(msg).with_traceback(e.__traceback__)

    return VersionInfo(*version)


if TYPE_CHECKING:

    @T.final
    class VersionArgs(T.NamedTuple):
        action: T.Literal["version"]
        path: Path

    @T.final
    class RewriteArgs(T.NamedTuple):
        action: T.Literal["rewrite"]
        path: Path
        version: str | None

    def make_args() -> "VersionArgs | RewriteArgs": ...
else:

    def make_args():
        return None


def main() -> None | int:
    import argparse

    parser = argparse.ArgumentParser()
    repo = parser.add_argument(
        "-C", dest="path", type=Path, required=True, help="Path to git repo root."
    )
    subparsers = parser.add_subparsers(dest="action", required=True)

    _ = subparsers.add_parser(
        "version", help="Generate version from git info or timestamp."
    )

    if "MESON_SOURCE_ROOT" in os.environ:
        repo.default = Path(os.environ["MESON_SOURCE_ROOT"]).parent
        repo.required = False

    rewrite = subparsers.add_parser(
        "rewrite",
        help="Rewrite meson.build with a generated version number",
    )
    _ = rewrite.add_argument("version", type=str, nargs="?")

    args = parser.parse_args(namespace=make_args())

    if args.action == "version":
        print(_calculate_version(args.path))
    elif args.action == "rewrite":
        import shlex
        import subprocess

        version = (
            VersionInfo.parse(args.version, full=True)
            if args.version
            else _calculate_version(args.path)
        )

        mesonrewrite = (
            shlex.split(os.environ["MESONREWRITE"])
            if "MESONWRITE" in os.environ
            else ["meson", "rewrite"]
        )
        return subprocess.run(
            [
                *mesonrewrite,
                "--sourcedir",
                os.environ.get("MESON_PROJECT_DIST_ROOT", args.path / "phnt"),
                "kwargs",
                "set",
                "project",
                "/",
                "version",
                str(version),
            ],
            executable=mesonrewrite[0],
            check=True,
        ).returncode


if __name__ == "__main__":
    quit(main())
