# Copyright (C) 2008  Kipp Cannon
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; either version 2 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
# Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.


#
# =============================================================================
#
#                                   Preamble
#
# =============================================================================
#


"""
Code to assist in reading and writing LAL time- and frequency series data
encoded in LIGO Light-Weight XML format.  The format recognized by the code
in this module is the same as generated by the array-related functions in
LAL's XML I/O code.  The format is also very similar to the format used by
the DMT to store time- and frequency-series data in XML files,
"""


import numpy


from glue.ligolw import ligolw
from glue.ligolw import array as ligolwarray
from glue.ligolw import param
from glue.ligolw import types as ligolwtypes
from pylal import git_version
from pylal.xlal.datatypes.complex16frequencyseries import COMPLEX16FrequencySeries
from pylal.xlal.datatypes.complex16timeseries import COMPLEX16TimeSeries
from pylal.xlal.datatypes.lalunit import LALUnit
from pylal.xlal.datatypes.ligotimegps import LIGOTimeGPS
from pylal.xlal.datatypes.real8frequencyseries import REAL8FrequencySeries
from pylal.xlal.datatypes.real8timeseries import REAL8TimeSeries


Attributes = ligolw.sax.xmlreader.AttributesImpl


__author__ = "Kipp Cannon <kipp.cannon@ligo.org>"
__version__ = "git id %s" % git_version.id
__date__ = git_version.date


#
# =============================================================================
#
#                                   XML I/O
#
# =============================================================================
#


#
# COMPLEX16FrequencySeries
#


def build_COMPLEX16FrequencySeries(series, comment = None):
	elem = ligolw.LIGO_LW(Attributes({u"Name": u"COMPLEX16FrequencySeries"}))
	if comment is not None:
		elem.appendChild(ligolw.Comment()).pcdata = comment
	# FIXME:  make Time class smart so we don't have to build it by
	# hand
	elem.appendChild(ligolw.Time(Attributes({u"Name": u"epoch", u"Type": u"GPS"}))).pcdata = unicode(series.epoch)
	elem.appendChild(param.from_pyvalue(u"f0", series.f0, unit = LALUnit("s^-1")))
	data = series.data
	data = numpy.row_stack((numpy.arange(0, len(data)) * series.deltaF, numpy.real(data), numpy.imag(data)))
	a = ligolwarray.from_array(series.name, data, dim_names = (u"Frequency", u"Frequency,Real,Imaginary"))
	a.setAttribute(u"Unit", unicode(series.sampleUnits))
	dim0 = a.getElementsByTagName(ligolw.Dim.tagName)[0]
	dim0.setAttribute(u"Unit", unicode(LALUnit("s^-1")))
	dim0.setAttribute(u"Start", ligolwtypes.FormatFunc[u"real_8"](series.f0))
	dim0.setAttribute(u"Scale", ligolwtypes.FormatFunc[u"real_8"](series.deltaF))
	elem.appendChild(a)
	return elem


def parse_COMPLEX16FrequencySeries(elem):
	t, = elem.getElementsByTagName(ligolw.Time.tagName)
	a, = elem.getElementsByTagName(ligolw.Array.tagName)
	dims = a.getElementsByTagName(ligolw.Dim.tagName)
	f0 = param.get_param(elem, u"f0")
	return COMPLEX16FrequencySeries(
		name = a.getAttribute(u"Name"),
		# FIXME:  make Time class smart so we don't have to parse
		# it by hand
		epoch = LIGOTimeGPS(t.pcdata),
		f0 = f0.pcdata * float(LALUnit(f0.Unit) / LALUnit("s^-1")),
		deltaF = float(dims[0].getAttribute(u"Scale")) * float(LALUnit(dims[0].getAttribute(u"Unit")) / LALUnit("s^-1")),
		sampleUnits = LALUnit(a.getAttribute(u"Unit")),
		data = a.array[1] + 1j * a.array[2]
	)


#
# COMPLEX16TimeSeries
#


def build_COMPLEX16TimeSeries(series, comment = None):
	elem = ligolw.LIGO_LW(Attributes({u"Name": u"COMPLEX16TimeSeries"}))
	if comment is not None:
		elem.appendChild(ligolw.Comment()).pcdata = comment
	# FIXME:  make Time class smart so we don't have to build it by
	# hand
	elem.appendChild(ligolw.Time(Attributes({u"Name": u"epoch", u"Type": u"GPS"}))).pcdata = unicode(series.epoch)
	elem.appendChild(param.from_pyvalue(u"f0", series.f0, unit = LALUnit("s^-1")))
	data = series.data
	data = numpy.row_stack((numpy.arange(0, len(data)) * series.deltaT, numpy.real(data), numpy.imag(data)))
	a = ligolwarray.from_array(series.name, data, dim_names = (u"Time", u"Time,Real,Imaginary"))
	a.setAttribute(u"Unit", unicode(series.sampleUnits))
	dim0 = a.getElementsByTagName(ligolw.Dim.tagName)[0]
	dim0.setAttribute(u"Unit", unicode(LALUnit("s")))
	dim0.setAttribute(u"Start", ligolwtypes.FormatFunc[u"real_8"](series.f0))
	dim0.setAttribute(u"Scale", ligolwtypes.FormatFunc[u"real_8"](series.deltaT))
	elem.appendChild(a)
	return elem


def parse_COMPLEX16TimeSeries(elem):
	t, = elem.getElementsByTagName(ligolw.Time.tagName)
	a, = elem.getElementsByTagName(ligolw.Array.tagName)
	dims = a.getElementsByTagName(ligolw.Dim.tagName)
	f0 = param.get_param(elem, u"f0")
	return COMPLEX16TimeSeries(
		name = a.getAttribute(u"Name"),
		# FIXME:  make Time class smart so we don't have to parse
		# it by hand
		epoch = LIGOTimeGPS(t.pcdata),
		f0 = f0.pcdata * float(LALUnit(f0.Unit) / LALUnit("s^-1")),
		deltaT = float(dims[0].getAttribute(u"Scale")) * float(LALUnit(dims[0].getAttribute(u"Unit")) / LALUnit("s")),
		sampleUnits = LALUnit(a.getAttribute(u"Unit")),
		data = a.array[1] + 1j * a.array[2]
	)


#
# REAL8FrequencySeries
#


def build_REAL8FrequencySeries(series, comment = None):
	elem = ligolw.LIGO_LW(Attributes({u"Name": u"REAL8FrequencySeries"}))
	if comment is not None:
		elem.appendChild(ligolw.Comment()).pcdata = comment
	# FIXME:  make Time class smart so we don't have to build it by
	# hand
	elem.appendChild(ligolw.Time(Attributes({u"Name": u"epoch", u"Type": u"GPS"}))).pcdata = unicode(series.epoch)
	elem.appendChild(param.from_pyvalue(u"f0", series.f0, unit = LALUnit("s^-1")))
	data = series.data
	data = numpy.row_stack((numpy.arange(0, len(data)) * series.deltaF, data))
	a = ligolwarray.from_array(series.name, data, dim_names = (u"Frequency", u"Frequency,Real"))
	a.setAttribute(u"Unit", unicode(series.sampleUnits))
	dim0 = a.getElementsByTagName(ligolw.Dim.tagName)[0]
	dim0.setAttribute(u"Unit", unicode(LALUnit("s^-1")))
	dim0.setAttribute(u"Start", ligolwtypes.FormatFunc[u"real_8"](series.f0))
	dim0.setAttribute(u"Scale", ligolwtypes.FormatFunc[u"real_8"](series.deltaF))
	elem.appendChild(a)
	return elem


def parse_REAL8FrequencySeries(elem):
	t, = elem.getElementsByTagName(ligolw.Time.tagName)
	a, = elem.getElementsByTagName(ligolw.Array.tagName)
	dims = a.getElementsByTagName(ligolw.Dim.tagName)
	f0 = param.get_param(elem, u"f0")
	return REAL8FrequencySeries(
		name = a.getAttribute(u"Name"),
		# FIXME:  make Time class smart so we don't have to parse
		# it by hand
		epoch = LIGOTimeGPS(t.pcdata),
		f0 = f0.pcdata * float(LALUnit(f0.Unit) / LALUnit("s^-1")),
		deltaF = float(dims[0].getAttribute(u"Scale")) * float(LALUnit(dims[0].getAttribute(u"Unit")) / LALUnit("s^-1")),
		sampleUnits = LALUnit(a.getAttribute(u"Unit")),
		data = a.array[1]
	)


#
# REAL8TimeSeries
#


def build_REAL8TimeSeries(series, comment = None):
	elem = ligolw.LIGO_LW(Attributes({u"Name": u"REAL8TimeSeries"}))
	if comment is not None:
		elem.appendChild(ligolw.Comment()).pcdata = comment
	# FIXME:  make Time class smart so we don't have to build it by
	# hand
	elem.appendChild(ligolw.Time(Attributes({u"Name": u"epoch", u"Type": u"GPS"}))).pcdata = unicode(series.epoch)
	elem.appendChild(param.from_pyvalue(u"f0", series.f0, unit = LALUnit("s^-1")))
	data = series.data
	data = numpy.row_stack((numpy.arange(0, len(data)) * series.deltaT, data))
	a = ligolwarray.from_array(series.name, data, dim_names = (u"Time", u"Time,Real"))
	a.setAttribute(u"Unit", unicode(series.sampleUnits))
	dim0 = a.getElementsByTagName(ligolw.Dim.tagName)[0]
	dim0.setAttribute(u"Unit", unicode(LALUnit("s")))
	dim0.setAttribute(u"Start", ligolwtypes.FormatFunc[u"real_8"](series.f0))
	dim0.setAttribute(u"Scale", ligolwtypes.FormatFunc[u"real_8"](series.deltaT))
	elem.appendChild(a)
	return elem


def parse_REAL8TimeSeries(elem):
	t, = elem.getElementsByTagName(ligolw.Time.tagName)
	a, = elem.getElementsByTagName(ligolw.Array.tagName)
	dims = a.getElementsByTagName(ligolw.Dim.tagName)
	f0 = param.get_param(elem, u"f0")
	return REAL8TimeSeries(
		name = a.getAttribute(u"Name"),
		# FIXME:  make Time class smart so we don't have to parse
		# it by hand
		epoch = LIGOTimeGPS(t.pcdata),
		f0 = f0.pcdata * float(LALUnit(f0.Unit) / LALUnit("s^-1")),
		deltaT = float(dims[0].getAttribute(u"Scale")) * float(LALUnit(dims[0].getAttribute(u"Unit")) / LALUnit("s")),
		sampleUnits = LALUnit(a.getAttribute(u"Unit")),
		data = a.array[1]
	)


#
# =============================================================================
#
#                                 XML PSD I/O
#
# =============================================================================
#


def make_psd_xmldoc(psddict, xmldoc = None):
	"""
	Construct an XML document tree representation of a dictionary of
	frequency series objects containing PSDs.  See also
	read_psd_xmldoc() for a function to parse the resulting XML
	documents.

	If xmldoc is None (the default), then a new XML document is created
	and the PSD dictionary added to it.  If xmldoc is not None then the
	PSD dictionary is appended to the children of that element inside a
	new LIGO_LW element.
	"""
	if xmldoc is None:
		xmldoc = ligolw.Document()
	lw = xmldoc.appendChild(ligolw.LIGO_LW())
	for instrument, psd in psddict.items():
		fs = lw.appendChild(build_REAL8FrequencySeries(psd))
		if instrument is not None:
			fs.appendChild(param.from_pyvalue(u"instrument", instrument))
	return xmldoc


def read_psd_xmldoc(xmldoc):
	"""
	Parse a dictionary of PSD frequency series objects from an XML
	document.  See also make_psd_xmldoc() for the construction of XML documents
	from a dictionary of PSDs.  Interprets an empty freuency series for an
	instrument as None.
	"""
	result = dict((param.get_pyvalue(elem, u"instrument"), parse_REAL8FrequencySeries(elem)) for elem in xmldoc.getElementsByTagName(ligolw.LIGO_LW.tagName) if elem.hasAttribute(u"Name") and elem.getAttribute(u"Name") == u"REAL8FrequencySeries")
	# Interpret empty frequency series as None
	for instrument in result:
		if len(result[instrument].data) == 0:
			result[instrument] = None
	return result
