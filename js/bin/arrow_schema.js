#! /usr/bin/env node

// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

var fs = require('fs');
var process = require('process');
var arrow = require('../lib/arrow.js');

var buf = fs.readFileSync(process.argv[process.argv.length - 1]);
var reader = arrow.getReader(buf);
console.log(JSON.stringify(reader.getSchema(), null, '\t'));
//console.log(JSON.stringify(reader.getVectors(), null, '\t'));
console.log('block count: ' + reader.getBatchCount());

