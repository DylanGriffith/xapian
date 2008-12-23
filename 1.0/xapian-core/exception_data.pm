# exception_data.pm: details of the exception heirarchy used by Xapian.
#
# Copyright (C) 2003,2004,2006,2007,2008 Olly Betts
# Copyright (C) 2007 Richard Boulton
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

package exception_data;
use Exporter;
@ISA = qw(Exporter);
@EXPORT = qw($copyright $generated_warning @baseclasses @classes %subclasses);

$copyright = <<'EOF';
/* Copyright (C) 2003,2004,2006,2007 Olly Betts
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */
EOF

$generated_warning =
"/* Warning: This file is generated by $0 - do not modify directly! */\n";

@baseclasses = ();
@classes = ();
%subclasses = ();

sub errorbaseclass {
    push @baseclasses, join("\t", @_);
    push @{$subclasses{$_[1]}}, $_[0];
}

sub errorclass {
    push @classes, join("\t", @_);
    push @{$subclasses{$_[1]}}, $_[0];
}

errorbaseclass('LogicError', 'Error', <<'DOC');
/** The base class for exceptions indicating errors in the program logic.
 *
 *  A subclass of LogicError will be thrown if Xapian detects a violation
 *  of a class invariant or a logical precondition or postcondition, etc.
 */
DOC

errorclass('AssertionError', 'LogicError', <<'DOC');
/** AssertionError is thrown if a logical assertion inside Xapian fails.
 *
 *  In a debug build of Xapian, a failed assertion in the core library code
 *  will cause AssertionError to be thrown.
 *
 *  This represents a bug in Xapian (either an invariant, precondition, etc
 *  has been violated, or the assertion is incorrect!)
 */
DOC

errorclass('InvalidArgumentError', 'LogicError', <<'DOC');
/** InvalidArgumentError indicates an invalid parameter value was passed to the API.
*/
DOC

errorclass('InvalidOperationError', 'LogicError', <<'DOC');
/** InvalidOperationError indicates the API was used in an invalid way.
 */
DOC

errorclass('UnimplementedError', 'LogicError', <<'DOC');
/** UnimplementedError indicates an attempt to use an unimplemented feature. */
DOC

# RuntimeError and subclasses:

errorbaseclass('RuntimeError', 'Error', <<'DOC');
/** The base class for exceptions indicating errors only detectable at runtime.
 *
 *  A subclass of RuntimeError will be thrown if Xapian detects an error
 *  which is exception derived from RuntimeError is thrown when an
 *  error is caused by problems with the data or environment rather
 *  than a programming mistake.
 */
DOC

errorclass('DatabaseError', 'RuntimeError', <<'DOC');
/** DatabaseError indicates some sort of database related error. */
DOC

errorclass('DatabaseCorruptError', 'DatabaseError', <<'DOC');
/** DatabaseCorruptError indicates database corruption was detected. */
DOC

errorclass('DatabaseCreateError', 'DatabaseError', <<'DOC');
/** DatabaseCreateError indicates a failure to create a database. */
DOC

errorclass('DatabaseLockError', 'DatabaseError', <<'DOC');
/** DatabaseLockError indicates failure to lock a database. */
DOC

errorclass('DatabaseModifiedError', 'DatabaseError', <<'DOC');
/** DatabaseModifiedError indicates a database was modified.
 *
 *  To recover after catching this error, you need to call
 *  Xapian::Database::reopen() on the Database and repeat the operation
 *  which failed.
 */
DOC

errorclass('DatabaseOpeningError', 'DatabaseError', <<'DOC');
/** DatabaseOpeningError indicates failure to open a database. */
DOC

errorclass('DatabaseVersionError', 'DatabaseOpeningError', <<'DOC');
/** DatabaseVersionError indicates that a database is in an unsupported format.
 *
 *  From time to time, new versions of Xapian will require the database format
 *  to be changed, to allow new information to be stored or new optimisations
 *  to be performed.  Backwards compatibility will sometimes be maintained, so
 *  that new versions of Xapian can open old databases, but in some cases
 *  Xapian will be unable to open a database because it is in too old (or new)
 *  a format.  This can be resolved either be upgrading or downgrading the
 *  version of Xapian in use, or by rebuilding the database from scratch with
 *  the current version of Xapian.
 */
DOC

errorclass('DocNotFoundError', 'RuntimeError', <<'DOC');
/** Indicates an attempt to access a document not present in the database. */
DOC

errorclass('FeatureUnavailableError', 'RuntimeError', <<'DOC');
/** Indicates an attempt to use a feature which is unavailable.
 *
 *  Typically a feature is unavailable because it wasn't compiled in, or
 *  because it requires other software or facilities which aren't available.
 */
DOC

errorclass('InternalError', 'RuntimeError', <<'DOC');
/** InternalError indicates a runtime problem of some sort. */
DOC

errorclass('NetworkError', 'RuntimeError', <<'DOC');
/** Indicates a problem communicating with a remote database. */
DOC

errorclass('NetworkTimeoutError', 'NetworkError', <<'DOC');
/** Indicates a timeout expired while communicating with a remote database. */
DOC

errorclass('QueryParserError', 'RuntimeError', <<'DOC');
/** Indicates a query string can't be parsed. */
DOC

errorclass('RangeError', 'RuntimeError', <<'DOC');
/** RangeError indicates an attempt to access outside the bounds of a container.
 */
DOC

1;
