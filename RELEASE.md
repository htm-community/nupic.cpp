# Release Process

## Download releases

We do tagged [releases for GitHub](https://github.com/htm-community/htm.core/releases). There you have both C++ library (and all needed examples, tests) as well as installable
python `.whl`. 

Prebuild python packages are also published to [PyPI htm.core](https://test.pypi.org/project/htm.core), and can be conveniently installed `pip install -i https://test.pypi.org/simple/ htm.core`. 

Each PR also uploads github Artifacts which can be downloaded for testing purposes. 

## Creating a new Release

Intended for maintainers of the repository. If you think you need a new release, please notify us in GH Issues. 

- we use semantic versioning `v<MAJOR>.<MINOR>.<BUGFIX>` version notation. 
- the release process is fully automated and done by the [CI release workflow](./.github/workflows/release.yml). 

### Make a new release from git

0. at `master` update tags: `git checkout master && git fetch --tags origin`
1. create a new tag (must increase) `git tag v2.1.42` (the format `vX.Y.Z` is mandatory)
2. push the tag to master, that starts the release builds `git push --tags origin`
3. enjoy :-) 

### Make a new release from GH web interface

The release build should also trigger when using the [WEB interface to create a new release](https://github.com/htm-community/htm.core/releases/new). 


