.. Licensed to the Apache Software Foundation (ASF) under one
.. or more contributor license agreements.  See the NOTICE file
.. distributed with this work for additional information
.. regarding copyright ownership.  The ASF licenses this file
.. to you under the Apache License, Version 2.0 (the
.. "License"); you may not use this file except in compliance
.. with the License.  You may obtain a copy of the License at

..   http://www.apache.org/licenses/LICENSE-2.0

.. Unless required by applicable law or agreed to in writing,
.. software distributed under the License is distributed on an
.. "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
.. KIND, either express or implied.  See the License for the
.. specific language governing permissions and limitations
.. under the License.

.. currentmodule:: pyarrow.csv
.. _csv:

Reading CSV files
=================

Arrow provides preliminary support for reading data from CSV files.
The features currently offered are the following:

* multi-threaded or single-threaded reading
* automatic decompression of input files (based on the filename extension,
  such as ``my_data.csv.gz``)
* fetching column names from the first row in the CSV file
* column-wise type inference and conversion to one of ``null``, ``int64``,
  ``float64`` or ``binary`` data
* detecting various spellings of null values such as ``NaN`` or ``#N/A``

Usage
-----

CSV reading functionality is available through the :mod:`pyarrow.csv` module.
In many cases, you will simply call the :func:`read_csv` function
with the file path you want to read from::

   >>> from pyarrow import csv
   >>> fn = 'tips.csv.gz'
   >>> table = csv.read_csv(fn)
   >>> table
   pyarrow.Table
   total_bill: double
   tip: double
   sex: binary
   smoker: binary
   day: binary
   time: binary
   size: int64
   >>> len(table)
   244
   >>> df = table.to_pandas()
   >>> df.head()
      total_bill   tip        sex smoker     day       time  size
   0       16.99  1.01  b'Female'  b'No'  b'Sun'  b'Dinner'     2
   1       10.34  1.66    b'Male'  b'No'  b'Sun'  b'Dinner'     3
   2       21.01  3.50    b'Male'  b'No'  b'Sun'  b'Dinner'     3
   3       23.68  3.31    b'Male'  b'No'  b'Sun'  b'Dinner'     2
   4       24.59  3.61  b'Female'  b'No'  b'Sun'  b'Dinner'     4

Customized parsing
------------------

To alter the default parsing settings in case of reading CSV files with an
unusual structure, you should create a :class:`ParseOptions` instance
and pass it to :func:`read_csv`.

Limitations
-----------

Arrow is not able to detect or convert other data types (such as dates
and times) than the four mentioned above.  It is also not possible to
choose the data types of columns explicitly.

Performance
-----------

Due to the structure of CSV files, one cannot expect the same levels of
performance as when reading dedicated binary formats like
:ref:`Parquet <Parquet>`.  Nevertheless, Arrow strives to reduce the
overhead of reading CSV files.

Performance options can be controlled through the :class:`ReadOptions` class.
Multi-threaded reading is the default for highest performance, distributing
the workload efficiently over all available cores.
