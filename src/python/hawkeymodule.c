/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <Python.h>

// hawkey
#include "src/advisory.h"
#include "src/advisorypkg.h"
#include "src/advisoryref.h"
#include "src/goal.h"
#include "src/package.h"
#include "src/query.h"
#include "src/subject.h"
#include "src/types.h"
#include "src/util.h"
#include "src/version.h"

// pyhawkey
#include "advisory-py.h"
#include "advisorypkg-py.h"
#include "advisoryref-py.h"
#include "exception-py.h"
#include "goal-py.h"
#include "nevra-py.h"
#include "package-py.h"
#include "packagedelta-py.h"
#include "possibilities-py.h"
#include "query-py.h"
#include "reldep-py.h"
#include "repo-py.h"
#include "sack-py.h"
#include "selector-py.h"
#include "subject-py.h"

#include "pycomp.h"

static PyObject *
detect_arch(PyObject *unused, PyObject *args)
{
    char *arch;

    if (ret2e(hy_detect_arch(&arch), "Failed detecting architecture."))
	return NULL;
    return PyString_FromString(arch);
}

static PyObject *
chksum_name(PyObject *unused, PyObject *args)
{
    int i;
    const char *name;

    if (!PyArg_ParseTuple(args, "i", &i))
	return NULL;
    name = hy_chksum_name(i);
    if (name == NULL) {
	PyErr_Format(PyExc_ValueError, "unrecognized chksum type: %d", i);
	return NULL;
    }

    return PyString_FromString(name);
}

static PyObject *
chksum_type(PyObject *unused, PyObject *str_o)
{
    PyObject *tmp_py_str = NULL;
    const char *str = pycomp_get_string(str_o, &tmp_py_str);

    if (str == NULL) {
        Py_XDECREF(tmp_py_str);
        return NULL;
    }
    int type = hy_chksum_type(str);
    Py_XDECREF(tmp_py_str);

    if (type == 0) {
	PyErr_Format(PyExc_ValueError, "unrecognized chksum type: %s", str);
	return NULL;
    }
    return PyLong_FromLong(type);
}

static PyObject *
split_nevra(PyObject *unused, PyObject *nevra_o)
{
    PyObject *tmp_py_str = NULL;
    const char *nevra = pycomp_get_string(nevra_o, &tmp_py_str);

    if (nevra == NULL) {
        Py_XDECREF(tmp_py_str);
        return NULL;
    }
    long epoch;
    char *name, *version, *release, *arch;

    int split_nevra_ret = hy_split_nevra(nevra, &name, &epoch, &version, &release, &arch);
    Py_XDECREF(tmp_py_str); // release memory after unicode string conversion

    if (ret2e(split_nevra_ret, "Failed parsing NEVRA."))
	return NULL;

    PyObject *ret = Py_BuildValue("slsss", name, epoch, version, release, arch);
    if (ret == NULL)
	return NULL;
    return ret;
}

static struct PyMethodDef hawkey_methods[] = {
    {"chksum_name",		(PyCFunction)chksum_name,
     METH_VARARGS,	NULL},
    {"chksum_type",		(PyCFunction)chksum_type,
     METH_O,		NULL},
    {"detect_arch",		(PyCFunction)detect_arch,
     METH_NOARGS,	NULL},
    {"split_nevra",		(PyCFunction)split_nevra,
     METH_O,		NULL},
    {NULL}				/* sentinel */
};

PYCOMP_MOD_INIT(_hawkey)
{
    PyObject *m;
    PYCOMP_MOD_DEF(m, "_hawkey", hawkey_methods)

    if (!m)
        return PYCOMP_MOD_ERROR_VAL;
    /* exceptions */
    if (!init_exceptions())
        return PYCOMP_MOD_ERROR_VAL;
    PyModule_AddObject(m, "Exception", HyExc_Exception);
    PyModule_AddObject(m, "ValueException", HyExc_Value);
    PyModule_AddObject(m, "QueryException", HyExc_Query);
    PyModule_AddObject(m, "ArchException", HyExc_Arch);
    PyModule_AddObject(m, "RuntimeException", HyExc_Runtime);
    PyModule_AddObject(m, "ValidationException", HyExc_Validation);

    /* _hawkey.Sack */
    if (PyType_Ready(&sack_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&sack_Type);
    PyModule_AddObject(m, "Sack", (PyObject *)&sack_Type);
    /* _hawkey.Advisory */
    if (PyType_Ready(&advisory_Type) < 0)
	return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&advisory_Type);
    PyModule_AddObject(m, "Advisory", (PyObject *)&advisory_Type);
    /* _hawkey.AdvisoryPkg */
    if (PyType_Ready(&advisorypkg_Type) < 0)
	return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&advisorypkg_Type);
    PyModule_AddObject(m, "AdvisoryPkg", (PyObject *)&advisorypkg_Type);
    /* _hawkey.AdvisoryRef */
    if (PyType_Ready(&advisoryref_Type) < 0)
	return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&advisoryref_Type);
    PyModule_AddObject(m, "AdvisoryRef", (PyObject *)&advisoryref_Type);
    /* _hawkey.Goal */
    if (PyType_Ready(&goal_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&goal_Type);
    PyModule_AddObject(m, "Goal", (PyObject *)&goal_Type);
    /* _hawkey.Package */
    if (PyType_Ready(&package_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&package_Type);
    PyModule_AddObject(m, "Package", (PyObject *)&package_Type);
    /* _hawkey.PackageDelta */
    if (PyType_Ready(&packageDelta_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&package_Type);
    PyModule_AddObject(m, "PackageDelta", (PyObject *)&packageDelta_Type);
    /* _hawkey.Query */
    if (PyType_Ready(&query_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&query_Type);
    PyModule_AddObject(m, "Query", (PyObject *)&query_Type);
    /* _hawkey.Reldep */
    if (PyType_Ready(&reldep_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&reldep_Type);
    PyModule_AddObject(m, "Reldep", (PyObject *)&reldep_Type);
    /* _hawkey.Selector */
    if (PyType_Ready(&selector_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&selector_Type);
    PyModule_AddObject(m, "Selector", (PyObject *)&selector_Type);
    /* _hawkey.Repo */
    if (PyType_Ready(&repo_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&repo_Type);
    PyModule_AddObject(m, "Repo", (PyObject *)&repo_Type);
    /* _hawkey.NEVRA */
    if (PyType_Ready(&nevra_Type) < 0)
	return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&nevra_Type);
    PyModule_AddObject(m, "NEVRA", (PyObject *)&nevra_Type);
    /* _hawkey.Subject */
    if (PyType_Ready(&subject_Type) < 0)
	return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&subject_Type);
    PyModule_AddObject(m, "Subject", (PyObject *)&subject_Type);
    /* _hawkey._Possibilities */
    possibilities_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&possibilities_Type) < 0)
	return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&possibilities_Type);

    PyModule_AddIntConstant(m, "FORM_NEVRA", HY_FORM_NEVRA);
    PyModule_AddIntConstant(m, "FORM_NEVR", HY_FORM_NEVR);
    PyModule_AddIntConstant(m, "FORM_NEV", HY_FORM_NEV);
    PyModule_AddIntConstant(m, "FORM_NA", HY_FORM_NA);
    PyModule_AddIntConstant(m, "FORM_NAME", HY_FORM_NAME);

    PyModule_AddIntConstant(m, "VERSION_MAJOR", HY_VERSION_MAJOR);
    PyModule_AddIntConstant(m, "VERSION_MINOR", HY_VERSION_MINOR);
    PyModule_AddIntConstant(m, "VERSION_PATCH", HY_VERSION_PATCH);

    PyModule_AddStringConstant(m, "SYSTEM_REPO_NAME", HY_SYSTEM_REPO_NAME);
    PyModule_AddStringConstant(m, "CMDLINE_REPO_NAME", HY_CMDLINE_REPO_NAME);

    PyModule_AddIntConstant(m, "PKG", HY_PKG);
    PyModule_AddIntConstant(m, "PKG_ARCH", HY_PKG_ARCH);
    PyModule_AddIntConstant(m, "PKG_CONFLICTS", HY_PKG_CONFLICTS);
    PyModule_AddIntConstant(m, "PKG_DESCRIPTION", HY_PKG_DESCRIPTION);
    PyModule_AddIntConstant(m, "PKG_DOWNGRADABLE", HY_PKG_DOWNGRADABLE);
    PyModule_AddIntConstant(m, "PKG_DOWNGRADES", HY_PKG_DOWNGRADES);
    PyModule_AddIntConstant(m, "PKG_EMPTY", HY_PKG_EMPTY);
    PyModule_AddIntConstant(m, "PKG_EPOCH", HY_PKG_EPOCH);
    PyModule_AddIntConstant(m, "PKG_EVR", HY_PKG_EVR);
    PyModule_AddIntConstant(m, "PKG_FILE", HY_PKG_FILE);
    PyModule_AddIntConstant(m, "PKG_LATEST_PER_ARCH", HY_PKG_LATEST_PER_ARCH);
    PyModule_AddIntConstant(m, "PKG_LATEST", HY_PKG_LATEST);
    PyModule_AddIntConstant(m, "PKG_LOCATION", HY_PKG_LOCATION);
    PyModule_AddIntConstant(m, "PKG_NAME", HY_PKG_NAME);
    PyModule_AddIntConstant(m, "PKG_NEVRA", HY_PKG_NEVRA);
    PyModule_AddIntConstant(m, "PKG_OBSOLETES", HY_PKG_OBSOLETES);
    PyModule_AddIntConstant(m, "PKG_PROVIDES", HY_PKG_PROVIDES);
    PyModule_AddIntConstant(m, "PKG_RELEASE", HY_PKG_RELEASE);
    PyModule_AddIntConstant(m, "PKG_REPONAME", HY_PKG_REPONAME);
    PyModule_AddIntConstant(m, "PKG_REQUIRES", HY_PKG_REQUIRES);
    PyModule_AddIntConstant(m, "PKG_SOURCERPM", HY_PKG_SOURCERPM);
    PyModule_AddIntConstant(m, "PKG_SUMMARY", HY_PKG_SUMMARY);
    PyModule_AddIntConstant(m, "PKG_UPGRADABLE", HY_PKG_UPGRADABLE);
    PyModule_AddIntConstant(m, "PKG_UPGRADES", HY_PKG_UPGRADES);
    PyModule_AddIntConstant(m, "PKG_URL", HY_PKG_URL);
    PyModule_AddIntConstant(m, "PKG_VERSION", HY_PKG_VERSION);

    PyModule_AddIntConstant(m, "CHKSUM_MD5", HY_CHKSUM_MD5);
    PyModule_AddIntConstant(m, "CHKSUM_SHA1", HY_CHKSUM_SHA1);
    PyModule_AddIntConstant(m, "CHKSUM_SHA256", HY_CHKSUM_SHA256);
    PyModule_AddIntConstant(m, "CHKSUM_SHA512", HY_CHKSUM_SHA512);

    PyModule_AddIntConstant(m, "ICASE", HY_ICASE);
    PyModule_AddIntConstant(m, "EQ", HY_EQ);
    PyModule_AddIntConstant(m, "LT", HY_LT);
    PyModule_AddIntConstant(m, "GT", HY_GT);
    PyModule_AddIntConstant(m, "NEQ", HY_NEQ);
    PyModule_AddIntConstant(m, "NOT", HY_NOT);
    PyModule_AddIntConstant(m, "SUBSTR", HY_SUBSTR);
    PyModule_AddIntConstant(m, "GLOB", HY_GLOB);

    PyModule_AddIntConstant(m, "REASON_DEP", HY_REASON_DEP);
    PyModule_AddIntConstant(m, "REASON_USER", HY_REASON_USER);

    PyModule_AddIntConstant(m, "ADVISORY_UNKNOWN", HY_ADVISORY_UNKNOWN);
    PyModule_AddIntConstant(m, "ADVISORY_SECURITY", HY_ADVISORY_SECURITY);
    PyModule_AddIntConstant(m, "ADVISORY_BUGFIX", HY_ADVISORY_BUGFIX);
    PyModule_AddIntConstant(m, "ADVISORY_ENHANCEMENT", HY_ADVISORY_ENHANCEMENT);

    PyModule_AddIntConstant(m, "REFERENCE_UNKNOWN", HY_REFERENCE_UNKNOWN);
    PyModule_AddIntConstant(m, "REFERENCE_BUGZILLA", HY_REFERENCE_BUGZILLA);
    PyModule_AddIntConstant(m, "REFERENCE_CVE", HY_REFERENCE_CVE);
    PyModule_AddIntConstant(m, "REFERENCE_VENDOR", HY_REFERENCE_VENDOR);

    return PYCOMP_MOD_SUCCESS_VAL(m);
}
