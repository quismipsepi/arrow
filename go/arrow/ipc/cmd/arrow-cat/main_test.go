// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package main

import (
	"bytes"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"testing"

	"github.com/apache/arrow/go/arrow/array"
	"github.com/apache/arrow/go/arrow/internal/arrdata"
	"github.com/apache/arrow/go/arrow/ipc"
	"github.com/apache/arrow/go/arrow/memory"
)

func TestCatStream(t *testing.T) {
	for _, tc := range []struct {
		name string
		want string
	}{
		{
			name: "primitives",
			want: `record 1...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-1 (null) (null) -4 -5]
  col[2] "int16s": [-1 (null) (null) -4 -5]
  col[3] "int32s": [-1 (null) (null) -4 -5]
  col[4] "int64s": [-1 (null) (null) -4 -5]
  col[5] "uint8s": [1 (null) (null) 4 5]
  col[6] "uint16s": [1 (null) (null) 4 5]
  col[7] "uint32s": [1 (null) (null) 4 5]
  col[8] "uint64s": [1 (null) (null) 4 5]
  col[9] "float32s": [1 (null) (null) 4 5]
  col[10] "float64s": [1 (null) (null) 4 5]
record 2...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-11 (null) (null) -14 -15]
  col[2] "int16s": [-11 (null) (null) -14 -15]
  col[3] "int32s": [-11 (null) (null) -14 -15]
  col[4] "int64s": [-11 (null) (null) -14 -15]
  col[5] "uint8s": [11 (null) (null) 14 15]
  col[6] "uint16s": [11 (null) (null) 14 15]
  col[7] "uint32s": [11 (null) (null) 14 15]
  col[8] "uint64s": [11 (null) (null) 14 15]
  col[9] "float32s": [11 (null) (null) 14 15]
  col[10] "float64s": [11 (null) (null) 14 15]
record 3...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-21 (null) (null) -24 -25]
  col[2] "int16s": [-21 (null) (null) -24 -25]
  col[3] "int32s": [-21 (null) (null) -24 -25]
  col[4] "int64s": [-21 (null) (null) -24 -25]
  col[5] "uint8s": [21 (null) (null) 24 25]
  col[6] "uint16s": [21 (null) (null) 24 25]
  col[7] "uint32s": [21 (null) (null) 24 25]
  col[8] "uint64s": [21 (null) (null) 24 25]
  col[9] "float32s": [21 (null) (null) 24 25]
  col[10] "float64s": [21 (null) (null) 24 25]
`,
		},
		{
			name: "structs",
			want: `record 1...
  col[0] "struct_nullable": {[-1 (null) (null) -4 -5] ["111" (null) (null) "444" "555"]}
record 2...
  col[0] "struct_nullable": {[-11 (null) (null) -14 -15 -16 (null) -18] ["1" (null) (null) "4" "5" "6" (null) "8"]}
`,
		},
		{
			name: "lists",
			want: `record 1...
  col[0] "list_nullable": [[1 (null) (null) 4 5] [11 (null) (null) 14 15] [21 (null) (null) 24 25]]
record 2...
  col[0] "list_nullable": [[-1 (null) (null) -4 -5] [-11 (null) (null) -14 -15] [-21 (null) (null) -24 -25]]
record 3...
  col[0] "list_nullable": [[-1 (null) (null) -4 -5] (null) [-21 (null) (null) -24 -25]]
`,
		},
		{
			name: "strings",
			want: `record 1...
  col[0] "strings": ["1é" (null) (null) "4" "5"]
  col[1] "bytes": ["1é" (null) (null) "4" "5"]
record 2...
  col[0] "strings": ["11" (null) (null) "44" "55"]
  col[1] "bytes": ["11" (null) (null) "44" "55"]
record 3...
  col[0] "strings": ["111" (null) (null) "444" "555"]
  col[1] "bytes": ["111" (null) (null) "444" "555"]
`,
		},
	} {
		t.Run(tc.name, func(t *testing.T) {
			mem := memory.NewCheckedAllocator(memory.NewGoAllocator())
			defer mem.AssertSize(t, 0)

			fname := func() string {
				f, err := ioutil.TempFile("", "go-arrow-")
				if err != nil {
					t.Fatal(err)
				}
				defer f.Close()

				w := ipc.NewWriter(f, ipc.WithSchema(arrdata.Records[tc.name][0].Schema()), ipc.WithAllocator(mem))
				defer w.Close()

				for _, rec := range arrdata.Records[tc.name] {
					err = w.Write(rec)
					if err != nil {
						t.Fatal(err)
					}
				}

				err = w.Close()
				if err != nil {
					t.Fatal(err)
				}

				err = f.Close()
				if err != nil {
					t.Fatal(err)
				}

				return f.Name()
			}()
			defer os.Remove(fname)

			f, err := os.Open(fname)
			if err != nil {
				t.Fatal(err)
			}
			defer f.Close()

			w := new(bytes.Buffer)
			err = processStream(w, f)
			if err != nil {
				t.Fatal(err)
			}

			if got, want := w.String(), tc.want; got != want {
				t.Fatalf("invalid output:\ngot:\n%s\nwant:\n%s\n", got, want)
			}
		})
	}
}

func TestCatFile(t *testing.T) {
	for _, tc := range []struct {
		name   string
		want   string
		stream bool
	}{
		{
			stream: true,
			name:   "primitives",
			want: `record 1...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-1 (null) (null) -4 -5]
  col[2] "int16s": [-1 (null) (null) -4 -5]
  col[3] "int32s": [-1 (null) (null) -4 -5]
  col[4] "int64s": [-1 (null) (null) -4 -5]
  col[5] "uint8s": [1 (null) (null) 4 5]
  col[6] "uint16s": [1 (null) (null) 4 5]
  col[7] "uint32s": [1 (null) (null) 4 5]
  col[8] "uint64s": [1 (null) (null) 4 5]
  col[9] "float32s": [1 (null) (null) 4 5]
  col[10] "float64s": [1 (null) (null) 4 5]
record 2...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-11 (null) (null) -14 -15]
  col[2] "int16s": [-11 (null) (null) -14 -15]
  col[3] "int32s": [-11 (null) (null) -14 -15]
  col[4] "int64s": [-11 (null) (null) -14 -15]
  col[5] "uint8s": [11 (null) (null) 14 15]
  col[6] "uint16s": [11 (null) (null) 14 15]
  col[7] "uint32s": [11 (null) (null) 14 15]
  col[8] "uint64s": [11 (null) (null) 14 15]
  col[9] "float32s": [11 (null) (null) 14 15]
  col[10] "float64s": [11 (null) (null) 14 15]
record 3...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-21 (null) (null) -24 -25]
  col[2] "int16s": [-21 (null) (null) -24 -25]
  col[3] "int32s": [-21 (null) (null) -24 -25]
  col[4] "int64s": [-21 (null) (null) -24 -25]
  col[5] "uint8s": [21 (null) (null) 24 25]
  col[6] "uint16s": [21 (null) (null) 24 25]
  col[7] "uint32s": [21 (null) (null) 24 25]
  col[8] "uint64s": [21 (null) (null) 24 25]
  col[9] "float32s": [21 (null) (null) 24 25]
  col[10] "float64s": [21 (null) (null) 24 25]
`,
		},
		{
			name: "primitives",
			want: `version: V4
record 1/3...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-1 (null) (null) -4 -5]
  col[2] "int16s": [-1 (null) (null) -4 -5]
  col[3] "int32s": [-1 (null) (null) -4 -5]
  col[4] "int64s": [-1 (null) (null) -4 -5]
  col[5] "uint8s": [1 (null) (null) 4 5]
  col[6] "uint16s": [1 (null) (null) 4 5]
  col[7] "uint32s": [1 (null) (null) 4 5]
  col[8] "uint64s": [1 (null) (null) 4 5]
  col[9] "float32s": [1 (null) (null) 4 5]
  col[10] "float64s": [1 (null) (null) 4 5]
record 2/3...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-11 (null) (null) -14 -15]
  col[2] "int16s": [-11 (null) (null) -14 -15]
  col[3] "int32s": [-11 (null) (null) -14 -15]
  col[4] "int64s": [-11 (null) (null) -14 -15]
  col[5] "uint8s": [11 (null) (null) 14 15]
  col[6] "uint16s": [11 (null) (null) 14 15]
  col[7] "uint32s": [11 (null) (null) 14 15]
  col[8] "uint64s": [11 (null) (null) 14 15]
  col[9] "float32s": [11 (null) (null) 14 15]
  col[10] "float64s": [11 (null) (null) 14 15]
record 3/3...
  col[0] "bools": [true (null) (null) false true]
  col[1] "int8s": [-21 (null) (null) -24 -25]
  col[2] "int16s": [-21 (null) (null) -24 -25]
  col[3] "int32s": [-21 (null) (null) -24 -25]
  col[4] "int64s": [-21 (null) (null) -24 -25]
  col[5] "uint8s": [21 (null) (null) 24 25]
  col[6] "uint16s": [21 (null) (null) 24 25]
  col[7] "uint32s": [21 (null) (null) 24 25]
  col[8] "uint64s": [21 (null) (null) 24 25]
  col[9] "float32s": [21 (null) (null) 24 25]
  col[10] "float64s": [21 (null) (null) 24 25]
`,
		},
		{
			stream: true,
			name:   "structs",
			want: `record 1...
  col[0] "struct_nullable": {[-1 (null) (null) -4 -5] ["111" (null) (null) "444" "555"]}
record 2...
  col[0] "struct_nullable": {[-11 (null) (null) -14 -15 -16 (null) -18] ["1" (null) (null) "4" "5" "6" (null) "8"]}
`,
		},
		{
			name: "structs",
			want: `version: V4
record 1/2...
  col[0] "struct_nullable": {[-1 (null) (null) -4 -5] ["111" (null) (null) "444" "555"]}
record 2/2...
  col[0] "struct_nullable": {[-11 (null) (null) -14 -15 -16 (null) -18] ["1" (null) (null) "4" "5" "6" (null) "8"]}
`,
		},
		{
			stream: true,
			name:   "lists",
			want: `record 1...
  col[0] "list_nullable": [[1 (null) (null) 4 5] [11 (null) (null) 14 15] [21 (null) (null) 24 25]]
record 2...
  col[0] "list_nullable": [[-1 (null) (null) -4 -5] [-11 (null) (null) -14 -15] [-21 (null) (null) -24 -25]]
record 3...
  col[0] "list_nullable": [[-1 (null) (null) -4 -5] (null) [-21 (null) (null) -24 -25]]
`,
		},
		{
			name: "lists",
			want: `version: V4
record 1/3...
  col[0] "list_nullable": [[1 (null) (null) 4 5] [11 (null) (null) 14 15] [21 (null) (null) 24 25]]
record 2/3...
  col[0] "list_nullable": [[-1 (null) (null) -4 -5] [-11 (null) (null) -14 -15] [-21 (null) (null) -24 -25]]
record 3/3...
  col[0] "list_nullable": [[-1 (null) (null) -4 -5] (null) [-21 (null) (null) -24 -25]]
`,
		},
		{
			stream: true,
			name:   "strings",
			want: `record 1...
  col[0] "strings": ["1é" (null) (null) "4" "5"]
  col[1] "bytes": ["1é" (null) (null) "4" "5"]
record 2...
  col[0] "strings": ["11" (null) (null) "44" "55"]
  col[1] "bytes": ["11" (null) (null) "44" "55"]
record 3...
  col[0] "strings": ["111" (null) (null) "444" "555"]
  col[1] "bytes": ["111" (null) (null) "444" "555"]
`,
		},
		{
			name: "strings",
			want: `version: V4
record 1/3...
  col[0] "strings": ["1é" (null) (null) "4" "5"]
  col[1] "bytes": ["1é" (null) (null) "4" "5"]
record 2/3...
  col[0] "strings": ["11" (null) (null) "44" "55"]
  col[1] "bytes": ["11" (null) (null) "44" "55"]
record 3/3...
  col[0] "strings": ["111" (null) (null) "444" "555"]
  col[1] "bytes": ["111" (null) (null) "444" "555"]
`,
		},
	} {
		t.Run(fmt.Sprintf("%s-stream=%v", tc.name, tc.stream), func(t *testing.T) {
			mem := memory.NewCheckedAllocator(memory.NewGoAllocator())
			defer mem.AssertSize(t, 0)

			fname := func() string {
				f, err := ioutil.TempFile("", "go-arrow-")
				if err != nil {
					t.Fatal(err)
				}
				defer f.Close()

				var w interface {
					io.Closer
					Write(array.Record) error
				}

				switch {
				case tc.stream:
					w = ipc.NewWriter(f, ipc.WithSchema(arrdata.Records[tc.name][0].Schema()), ipc.WithAllocator(mem))
				default:
					w, err = ipc.NewFileWriter(f, ipc.WithSchema(arrdata.Records[tc.name][0].Schema()), ipc.WithAllocator(mem))
					if err != nil {
						t.Fatal(err)
					}
				}
				defer w.Close()

				for _, rec := range arrdata.Records[tc.name] {
					err = w.Write(rec)
					if err != nil {
						t.Fatal(err)
					}
				}

				err = w.Close()
				if err != nil {
					t.Fatal(err)
				}

				err = f.Close()
				if err != nil {
					t.Fatal(err)
				}

				return f.Name()
			}()
			defer os.Remove(fname)

			w := new(bytes.Buffer)
			err := processFile(w, fname)
			if err != nil {
				t.Fatal(err)
			}

			if got, want := w.String(), tc.want; got != want {
				t.Fatalf("invalid output:\ngot:\n%s\nwant:\n%s\n", got, want)
			}
		})
	}
}
