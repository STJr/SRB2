# libopenmpt Debian backport info

Backported libopenmpt 0.4.0 packages are available at ppa:stjr/srb2
for Ubuntu Disco, Cosmic, Bionic, Xenial, and Trusty as of 2019/01/04.

Debian Jessie users should use the Trusty package. Later Debian versions
may use Disco or another working version.

* libopenmpt 0.4.0 source package: http://archive.ubuntu.com/ubuntu/pool/universe/libo/libopenmpt/libopenmpt_0.4.0-1.dsc

# Backporting from Disco to Cosmic and Bionic

Cosmic and Bionic require no changes to the source package. They have
the prerequisite `debhelper` and `dpkg-dev` versions, a matching
`automake` version (1.15), as well as G++ > 5.

Use the `backportpackage` script to automatically tag the package to
Cosmic and Bionic, then upload to PPA:

```
sudo apt install ubuntu-dev-tools
export UBUMAIL="John Doe <email@com.com>" # Name and email associated with your PGP sign key
backportpackage -d [cosmic/bionic] -u [path-to-ppa] --key=[key-fingerprint] http://archive.ubuntu.com/ubuntu/pool/universe/libo/libopenmpt/libopenmpt_0.4.0-1.dsc
```

# Backporting from Disco to Xenial

Download the package:

```
dget http://archive.ubuntu.com/ubuntu/pool/universe/libo/libopenmpt/libopenmpt_0.4.0-1.dsc
```

Xenial has an earlier `debhelper` version, but the rest of the prerequisites
are available.

The required changes (see patch `debian/libopenmpt-0.4.0-xenial-backport.diff` in this directory):

* `debian/control`
    * Add `automake` to Build-Depends
    * Change `debhelper` version to `(>= 9.0~)`
* `debian/compat`
    * Change to `10`

Then run these commands:

```
dch -i # Edit the changelog; make sure the Name and email match the PGP sign key and that the changelog targets xenial
debuild -s -d -k0x[key-fingerprint] # Build the updated package
dput [path-to-ppa] [path-to-changes-file] # Upload to PPA
```

# Backporting from Disco to Trusty

Download the package:

```
dget http://archive.ubuntu.com/ubuntu/pool/universe/libo/libopenmpt/libopenmpt_0.4.0-1.dsc
```

Trusty requires more changes because it uses G++ 4.8 whereas the source
package expects G++ >= 5. Automake is an earlier version as well --
1.14 vs 1.15 -- so `autoreconf` needs to be run at build time.

The required changes (see patch `debian/libopenmpt-0.4.0-trusty-backport.diff` in this directory):

* `debian/control`
    * Add `automake` and `libtool` to Build-Depends
    * Change `debhelper` version to `(>= 9.0~)`
    * Change `dpkg-dev` version to `(>= 1.17.0)`
* `debian/compat`
    * Change to `10`
* `debian/rules`
    * Under `override_dh_auto_configure`, input this line as the first command:
      `autoreconf --force --install`
        * This re-configures the package for Automake 1.14
* `debian/libopenmpt-modplug1.symbols` and `debian/libopenmpt0.symbols`
    * Delete these files
        * The C++ ABI for G++ 4.8 is incompatible with G++ >= 5, so the
          generated symbols will be different than the expected symbols
          in the `*.symbols` files. Deleting these files will skip the
          symbol check.

Then run these commands:

```
dch -i # Edit the changelog; make sure the Name and email match the PGP sign key and that the changelog targets trusty
debuild -s -d -k0x[key-fingerprint] # Build the updated package
dput [path-to-ppa] [path-to-changes-file] # Upload to PPA
```
