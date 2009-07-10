#!/bin/bash
# Copyright (c) 2009 by Xuggle Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# It is REQUESTED BUT NOT REQUIRED if you use this library, that you let 
# us know by sending e-mail to info\@xuggle.com telling us briefly how you're
# using the library and what you like or don't like about it.

#
# This script creates a self-installing shell script from a binary
# distribution, with a license file, key, and product name
#
PRODUCT="$1"
LICENSE="$2"
LICENSEKEY="$3"
BINARIES="$4"
OUTFILE="$5"
if [ -z "$OUTFILE" ]; then
    echo "Usage: product_name license.txt license-key binary.tar.gz outputfile.sh"
    exit 1;
fi
echo "Creating installer; Product: ${PRODUCT}; License: ${LICENSEKEY};"

:> "$OUTFILE"
cat >> "$OUTFILE" <<_EOF_
#!/bin/bash
echo ""
echo "Installer: ${PRODUCT}"
echo "License Key: ${LICENSEKEY}"
echo ""
echo "Built by: http://www.xuggle.com"
echo ""
echo -n "Hit Enter to begin installation (must accept license first) or Ctrl-C to abort"
read ignore
more << "__END_LICENSE__"
_EOF_
cat < "$LICENSE" >> $OUTFILE
echo >> $OUTFILE
echo "License Key: ${LICENSEKEY}" >> $OUTFILE
cat >> "$OUTFILE" <<_EOF_
__END_LICENSE__

accepts=
while [ -z "\$accepts" ]; do
    echo
    echo -n "Do you agree to the license terms above? [yes or no]: "
    read firstword rest
    case \$firstword in
      [yY] | [yY][eE][sS])
        accepts=1
      ;;
      [nN] | [nN][oO])
        echo "You must agree to license terms to install ${PRODUCT}"
        exit 1;
    esac
done

installDir=/usr/local
if [ ! -z "\$XUGGLE_HOME" ]; then
    installDir=\$XUGGLE_HOME
fi
echo -n "Specify directory to install to: [\$installDir]: "
read userInstallDir
if [ -z "\$userInstallDir" ]; then
    userInstallDir=\$installDir
fi
if [ ! -d "\$userInstallDir" ]; then
    mkdir "\$userInstallDir"
fi
_EOF_

# Now prep the tar unbundler
cat >> "$OUTFILE" <<_EOF_

GET_TAR_LINENO=\`awk '/^__TARFILE_FOLLOWS__/ { print NR + 1; exit 0; }' \$0\`

#remember our file name
THIS=\$0


# take the tarfile and pipe it into tar
echo "Installing ${PRODUCT} to \$userInstallDir"
tail -n +\$GET_TAR_LINENO \$THIS | tar --directory \${userInstallDir} -xzv
if [ \$? -ne 0 ]; then
    echo
    echo "-------------------- ERROR -----------------------"
    echo "Failed to install ${PRODUCT}."
    echo "You may need to be root to install to \${userInstallDir}."
    echo "-------------------- ERROR -----------------------"
    echo
    exit 1
fi

#
# place any bash script here you need.
# Any script here will happen after the tar file extract.
echo
echo "Successfully Installed: ${PRODUCT} to \${userInstallDir}"
echo "  License Key: ${LICENSEKEY}"

SO_PATH_VAR=LD_LIBRARY_PATH
if uname | grep 'Darwin'>/dev/null; then
  SO_PATH_VAR=DYLD_LIBRARY_PATH
fi
echo
echo "To use, make sure the following environment variables are"
echo "always set  prior to running programs using this product"
echo " (for example, copy and paste these lines into ~/.profile)"
echo
echo "export XUGGLE_HOME=\$userInstallDir"
echo "export \$SO_PATH_VAR=\\\$XUGGLE_HOME/lib:\\\$\$SO_PATH_VAR"
echo "export PATH=\\\$XUGGLE_HOME/bin:\\\$PATH"
echo
exit 0

# NOTE: Don't place any newline characters after the last line below.
__TARFILE_FOLLOWS__
_EOF_
cat < "$BINARIES" >> "$OUTFILE"
chmod a+x "$OUTFILE"
