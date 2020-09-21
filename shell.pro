TEMPLATE = subdirs

load(configure)

SUBDIRS = shell package

package.depends += shell
