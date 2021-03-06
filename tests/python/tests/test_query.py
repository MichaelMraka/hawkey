#
# Copyright (C) 2012-2014 Red Hat, Inc.
#
# Licensed under the GNU Lesser General Public License Version 2.1
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

from __future__ import absolute_import
from . import base

import hawkey
import sys
import unittest

class TestQuery(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()

    def test_sanity(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__eq="flying")
        self.assertEqual(q.count(), 1)

    def test_creation_empty_sack(self):
        s = hawkey.Sack(make_cache_dir=True)
        q = hawkey.Query(s)

    def test_exception(self):
        q = hawkey.Query(self.sack)
        self.assertRaises(hawkey.ValueException, q.filter, flying__eq="name")
        self.assertRaises(hawkey.ValueException, q.filter, flying="name")

    def test_unicode(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__eq=u"flying")
        self.assertEqual(q.count(), 1)

        q = hawkey.Query(self.sack)
        q.filterm(name__eq=[u"flying", "penny"])
        self.assertEqual(q.count(), 2)

    def test_count(self):
        q = hawkey.Query(self.sack).filter(name=["flying", "penny"])

        self.assertIsNone(q.result)
        self.assertEqual(len(q), 2)
        self.assertIsNotNone(q.result)
        self.assertEqual(len(q), q.count())
        self.assertTrue(q)

        q = hawkey.Query(self.sack).filter(name="naturalE")
        self.assertFalse(q)
        self.assertIsNotNone(q.result)

    def test_kwargs_check(self):
        q = hawkey.Query(self.sack)
        self.assertRaises(hawkey.ValueException, q.filter,
                          name="flying", upgrades="maracas")

    def test_kwargs(self):
        q = hawkey.Query(self.sack)
        # test combining several criteria
        q.filterm(name__glob="*enny*", summary__substr="eyes")
        self.assertEqual(q.count(), 1)
        # test shortcutting for equality comparison type
        q = hawkey.Query(self.sack)
        q.filterm(name="flying")
        self.assertEqual(q.count(), 1)
        # test flags parsing
        q = hawkey.Query(self.sack).filter(name="FLYING")
        self.assertEqual(q.count(), 0)
        q = hawkey.Query(self.sack).filter(hawkey.ICASE, name="FLYING")
        self.assertEqual(q.count(), 1)

    def test_in(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["ool", "enny-li"])
        self.assertEqual(q.count(), 2)

    def test_in_set(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=set(["ool", "enny-li"]))
        self.assertEqual(q.count(), 2)

    def test_iteration(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["penny"])
        self.assertEqual(q.count(), 2)
        self.assertNotEqual(q[0], q[1])

    def test_clone(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["penny"])
        q_clone = hawkey.Query(query=q)
        del q

        self.assertEqual(q_clone.count(), 2)
        self.assertNotEqual(q_clone[0], q_clone[1])

    def test_clone_with_evaluation(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr="penny")
        q.run()
        q_clone = hawkey.Query(query=q)
        del q
        self.assertTrue(q_clone.evaluated)
        self.assertLength(q_clone.result, 2)

    def test_immutability(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q2 = q.filter(evr="5.0-0")
        self.assertEqual(q.count(), 2)
        self.assertEqual(q2.count(), 1)

    def test_copy_lazyness(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertIsNone(q.result)
        q2 = q.filter(evr="5.0-0")
        self.assertIsNone(q.result)

    def test_empty(self):
        q = hawkey.Query(self.sack).filter(empty=True)
        self.assertLength(q, 0)
        q = hawkey.Query(self.sack)
        self.assertRaises(hawkey.ValueException, q.filter, empty=False)

    def test_epoch(self):
        q = hawkey.Query(self.sack).filter(epoch__gt=4)
        self.assertEqual(len(q), 1)
        self.assertEqual(q[0].epoch, 6)

    def test_version(self):
        q = hawkey.Query(self.sack).filter(version__gte="5.0")
        self.assertEqual(len(q), 3)
        q = hawkey.Query(self.sack).filter(version__glob="1.2*")
        self.assertLength(q, 2)

    def test_package_in(self):
        pkgs = list(hawkey.Query(self.sack).filter(name=["flying", "penny"]))
        q = hawkey.Query(self.sack).filter(pkg=pkgs)
        self.assertEqual(len(q), 2)
        q2 = q.filter(version__gt="3")
        self.assertEqual(len(q2), 1)

    def test_nevra_match(self):
        query = hawkey.Query(self.sack).filter(nevra__glob="*lib*64")
        self.assertEqual(len(query), 1)
        self.assertEqual(str(query[0]), 'penny-lib-4-1.x86_64')

    def test_repeated(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q.filterm(latest_per_arch=True)
        self.assertEqual(len(q), 1)

    def test_latest(self):
        q = hawkey.Query(self.sack).filter(name="pilchard")
        q.filterm(latest_per_arch=True)
        self.assertEqual(len(q), 2)
        q.filterm(latest=True)
        self.assertEqual(len(q), 1)

    def test_reldep(self):
        flying = base.by_name(self.sack, "flying")
        requires = flying.requires
        q = hawkey.Query(self.sack).filter(provides=requires[0])
        self.assertEqual(len(q), 1)
        self.assertEqual(str(q[0]), "penny-lib-4-1.x86_64")

        self.assertRaises(hawkey.QueryException, q.filter,
                          provides__gt=requires[0])

    def test_reldep_list(self):
        self.sack.load_test_repo("updates", "updates.repo")
        fool = base.by_name_repo(self.sack, "fool", "updates")
        q = hawkey.Query(self.sack).filter(provides=fool.obsoletes)
        self.assertEqual(str(q.run()[0]), "penny-4-1.noarch")

    def test_disabled_repo(self):
        self.sack.disable_repo(hawkey.SYSTEM_REPO_NAME)
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertLength(q.run(), 0)
        self.sack.enable_repo(hawkey.SYSTEM_REPO_NAME)
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertLength(q.run(), 2)

    def test_multiple_flags(self):
        q = hawkey.Query(self.sack).filter(name__glob__not=["p*", "j*"])
        self.assertItemsEqual(list(map(lambda p: p.name, q.run())),
                              ['baby', 'dog', 'flying', 'fool', 'gun', 'tour'])


class TestQueryAllRepos(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.sack.load_test_repo("updates", "updates.repo")

    def test_requires(self):
        reldep = hawkey.Reldep(self.sack, "semolina = 2")
        q = hawkey.Query(self.sack).filter(requires=reldep)
        self.assertItemsEqual(list(map(str, q.run())),
                              ['walrus-2-5.noarch', 'walrus-2-6.noarch'])

        reldep = hawkey.Reldep(self.sack, "semolina > 1.0")
        q = hawkey.Query(self.sack).filter(requires=reldep)
        self.assertItemsEqual(list(map(str, q.run())),
                              ['walrus-2-5.noarch', 'walrus-2-6.noarch'])

    def test_obsoletes(self):
        reldep = hawkey.Reldep(self.sack, "penny < 4-0")
        q = hawkey.Query(self.sack).filter(obsoletes=reldep)
        self.assertItemsEqual(list(map(str, q.run())), ['fool-1-5.noarch'])

    def test_downgradable(self):
        query = hawkey.Query(self.sack).filter(downgradable=True)
        self.assertEqual({str(pkg) for pkg in query},
                         {'baby-6:5.0-11.x86_64', 'jay-5.0-0.x86_64'})

class TestQueryUpdates(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("updates", "updates.repo")

    def test_upgradable(self):
        query = hawkey.Query(self.sack).filter(upgradable=True)
        self.assertEqual({str(pkg) for pkg in query},
                         {'dog-1-1.x86_64', 'flying-2-9.noarch',
                          'fool-1-3.noarch', 'pilchard-1.2.3-1.i686',
                          'pilchard-1.2.3-1.x86_64'})

    def test_updates_noarch(self):
        q = hawkey.Query(self.sack)
        q.filterm(name="flying", upgrades=1)
        self.assertEqual(q.count(), 3)

    def test_updates_arch(self):
        q = hawkey.Query(self.sack)
        pilchard = q.filter(name="dog", upgrades=True)
        self.assertItemsEqual(list(map(str, pilchard.run())), ['dog-1-2.x86_64'])

    def test_glob_arch(self):
        q = hawkey.Query(self.sack)
        pilchard = q.filter(name="pilchard", version="1.2.4", release="1",
                            arch__glob="*6*")
        res = list(map(str, pilchard.run()))
        self.assertItemsEqual(res, ["pilchard-1.2.4-1.x86_64",
                              "pilchard-1.2.4-1.i686"])

    def test_obsoletes(self):
        q = hawkey.Query(self.sack).filter(name="penny")
        o = hawkey.Query(self.sack)
        self.assertRaises(hawkey.QueryException, o.filter, obsoletes__gt=q)
        self.assertRaises(hawkey.ValueException, o.filter, requires=q)

        o = hawkey.Query(self.sack).filter(obsoletes=q)
        self.assertLength(o, 1)
        self.assertEqual(str(o[0]), 'fool-1-5.noarch')

    def test_subquery_evaluated(self):
        q = hawkey.Query(self.sack).filter(name="penny")
        self.assertFalse(q.evaluated)
        self.assertIsNone(q.result)
        o = hawkey.Query(self.sack).filter(obsoletes=q)
        self.assertTrue(q.evaluated)
        self.assertIsInstance(q.result, list)
        self.assertLength(o, 1)

class TestOddArch(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_test_repo("ppc", "ppc.repo")

    def test_latest(self):
        q = hawkey.Query(self.sack).filter(latest=True)
        self.assertEqual(len(q), 1)

        q = hawkey.Query(self.sack).filter(latest_per_arch=True)
        self.assertEqual(len(q), 1)

class TestQuerySubclass(base.TestCase):
    class CustomQuery(hawkey.Query):
        pass

    def test_instance(self):
        sack = base.TestSack(repo_dir=self.repo_dir)
        q = self.CustomQuery(sack)
        self.assertIsInstance(q, self.CustomQuery)
        q = q.filter(name="pepper")
        self.assertIsInstance(q, self.CustomQuery)
