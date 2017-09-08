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

import { flatbuffers } from 'flatbuffers';
import * as Schema_ from '../format/Schema_generated';
import * as Message_ from '../format/Message_generated';

import { readFile } from './file';
import { readStream } from './stream';
import { readVector } from './vector';
import { Vector } from '../vector/vector';
import { readDictionaries } from './dictionary';

import ByteBuffer = flatbuffers.ByteBuffer;
export import Schema = Schema_.org.apache.arrow.flatbuf.Schema;
export import RecordBatch = Message_.org.apache.arrow.flatbuf.RecordBatch;
export type Dictionaries = { [k: string]: Vector<any> };
export type IteratorState = { nodeIndex: number; bufferIndex: number };

export function* readRecords(...bytes: ByteBuffer[]) {
    try {
        yield* readFile(...bytes);
    } catch (e) {
        try {
            yield* readStream(...bytes);
        } catch (e) {
            throw new Error('Invalid Arrow buffer');
        }
    }
}

export function* readBuffers(...bytes: Array<Uint8Array | Buffer | string>) {
    const dictionaries: Dictionaries = {};
    const byteBuffers = bytes.map(toByteBuffer);
    for (let { schema, batch } of readRecords(...byteBuffers)) {
        let vectors: Vector<any>[] = [];
        let state = { nodeIndex: 0, bufferIndex: 0 };
        let index = -1, fieldsLength = schema.fieldsLength();
        if (batch.id) {
            while (++index < fieldsLength) {
                for (let [id, vector] of readDictionaries(schema.fields(index), batch, state, dictionaries)) {
                    dictionaries[id] = dictionaries[id] && dictionaries[id].concat(vector) || vector;
                }
            }
        } else {
            while (++index < fieldsLength) {
                vectors[index] = readVector(schema.fields(index), batch, state, dictionaries);
            }
            yield vectors;
        }
    }
}

function toByteBuffer(bytes?: Uint8Array | Buffer | string) {
    let arr: Uint8Array = bytes as any || new Uint8Array(0);
    if (typeof bytes === 'string') {
        arr = new Uint8Array(bytes.length);
        for (let i = -1, n = bytes.length; ++i < n;) {
            arr[i] = bytes.charCodeAt(i);
        }
        return new ByteBuffer(arr);
    }
    return new ByteBuffer(arr);
}
