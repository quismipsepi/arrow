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

use std::convert::TryFrom;
use std::sync::Arc;

use arrow::array::Int32Array;
use arrow::datatypes::Schema;
use arrow::flight::flight_data_to_batch;
use flight::flight_service_client::FlightServiceClient;
use flight::Ticket;

#[tokio::main]
async fn main() -> Result<(), Box<dyn std::error::Error>> {
    let mut client = FlightServiceClient::connect("http://localhost:50051").await?;

    let request = tonic::Request::new(Ticket {
        ticket: "SELECT id FROM alltypes_plain".into(),
    });

    let mut stream = client.do_get(request).await?.into_inner();

    // the schema should be the first message returned, else client should error
    let flight_data = stream.message().await?.unwrap();
    // convert FlightData to a stream
    let schema = Arc::new(Schema::try_from(&flight_data)?);
    println!("Schema: {:?}", schema);

    // all the remaining stream messages should be dictionary and record batches
    while let Some(flight_data) = stream.message().await? {
        // the unwrap is infallible and thus safe
        let record_batch = flight_data_to_batch(&flight_data, schema.clone())?.unwrap();

        println!(
            "record_batch has {} columns and {} rows",
            record_batch.num_columns(),
            record_batch.num_rows()
        );
        let column = record_batch.column(0);
        let column = column
            .as_any()
            .downcast_ref::<Int32Array>()
            .expect("Unable to get column");
        println!("Column 1: {:?}", column);
    }

    Ok(())
}
