# Introduction
SymCrypt is the core cryptographic function library currently used by Windows.

## History
The library was started in late 2006 with the first sources committed in Feb 2007.
Initially the goal was limited to implement symmetric cryptographic operations, hence the name.
Starting with Windows 8, it has been the primary crypto library for symmetric algorithms.

In 2015 we started the work of adding asymmetric algorithms to SymCrypt. Since the 1703 release of Windows 10,
SymCrypt has been the primary crypto library for all algorithms in Windows.

## Goals
Like any engineering project, SymCrypt is a compromise between conflicting requirements:
- Provide safe implementations of the cryptographic algorithms needed by Microsoft products.
- Run on all CPU architectures supported by Windows.
- Good performance.
- Minimize maintenance cost.
- Support FIPS 140 certification of products using SymCrypt.
- Provide high assurance in the proper functionality of the library.

# Cloning the Repo
In some of our Linux modules, SymCrypt uses [Jitterentropy](https://github.com/smuellerDD/jitterentropy-library)
as a source of FIPS-certifiable entropy. To build these modules, you will need to ensure that the
jitterentropy-library submodule is also cloned. You can do this by running
`git submodule update --init` after cloning.

The `unittest/SymCryptDependencies` submodule provides the RSA32 and msbignum implementations which are used as
benchmarks in the unit tests when compiled on Windows. Due to licensing restrictions, we cannot release these
libraries publicly, so this submodule will only be cloneable by Microsoft employees with access to our private
Azure DevOps repository. If you are external to Microsoft, you can ignore this submodule. It is only used in
the unit tests and does not change the behavior of the SymCrypt product code.

# Building
The easiest way to get started building SymCrypt is to use the Python build script, `scripts/build.py`. You
can run it with the `--help` argument to get help about which arguments are required and what each one does. For
detailed build instructions, including alternative ways to build, see [BUILD.md](BUILD.md).

# Testing
The SymCrypt unit test runs extensive functional tests on the SymCrypt library. On Windows it also compares results
against on other implementations such as the Windows APIs CNG and CAPI, and the older crypto libraries rsa32 and
msbignum, if they are available. It also provides detailed performance information.

After a successful build, you can use the `scripts/test.py` helper script to run the unit tests.

# Versioning and Servicing
As of version 101.0.0, SymCrypt uses the version scheme defined by the
[Semantic Versioning 2.0.0](https://semver.org/spec/v2.0.0.html) specification. This means:

- Major version changes introduce ABI and/or API breaking changes (including behavior changes)
- Minor version changes introduce backwards compatible additional functionality or improvements, and/or bug fixes
- Patch version changes introduce backwards compatible bug fixes

The initial open source release started at version 100 for compatibility with our previous
internal versioning scheme.

Regarding servicing, our strong recommendation is that distro vendors and application developers regularly
update to the latest version of SymCrypt and SymCrypt engine for both security fixes and 
functionality/performance improvements. We take care to maintain a stable API and ABI for SymCrypt and have
a suite of strong regression tests, and staying on the current version prevents the need for complex
and potential riskier backports.

We will support long-term servicing of specific releases for security fixes. Details of this plan will be
released publicly in the future.

# Security Bugs
If you believe you have found a problem that affects the security of this code, please do **NOT** create an issue
or pull request, but instead email your comments to secure@microsoft.com. See [SECURITY.md](SECURITY.md) for more info.

# Contribute
We love to receive comments and suggestions. Unfortunately we cannot accept external code contributions except in
specific circumstances from vetted partners with whom we have a pre-arranged agreement.
Cryptographic code is considered highly sensitive by many of our large customers.
We have some very big customers who put great value in the assurance of the crypto code used in their organization.
By restricting the coding to a handful of employees we can greatly reduce the (perceived) risk of malicious contributions.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.


