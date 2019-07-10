/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// Package main implements a client for Greeter service.
package main

import (
	"context"
	"log"
	"fmt"
	"time"

	"google.golang.org/grpc"
	pb "google.golang.org/grpc/examples/go_client/membership"
)

const (
	address     = "localhost:50051"
)

func main() {
	start := time.Now()

	// Set up a connection to the server.
	conn, err := grpc.Dial(address, grpc.WithInsecure())
	if err != nil {
		log.Fatalf("did not connect: %v", err)
	}
	defer conn.Close()
	c := pb.NewMemRequestClient(conn)

	const read int32 = 1 // set 1 for ReadOnly else set 0 for update requests.

	//Read only,
    request_membership1 := [6]string{`{"Mode": "public_keys", "BGID": "0"}`,
    								 `{"Mode": "quorum_size", "BGID": "0"}`,
    								  `{"Mode": "byzantine_groups", "BGID": "0"}`,
    								   `{"Mode": "emulators", "SLID": "0", "height":"2"}`,
    								    `{"Mode": "emulators", "SLID": "4", "height":"1"}`,
    								    `{"Mode": "super_leafs", "SLID": "0"}`}

	for i := 0; i < 6; i++ {
		// sum += i
	    fmt.Println("\n----- Request : ", request_membership1[i])
		ctx, cancel := context.WithTimeout(context.Background(), time.Second)
		defer cancel()
		r, err := c.SayHello(ctx, &pb.RequestMembership{Name: request_membership1[i], ReadOnly: read})
		if err != nil {
			log.Fatalf("could not connect: %v", err)
		}
		fmt.Println("\n", r.Message)
	}
	elapsed := time.Since(start)
	fmt.Println("Time taken: ",elapsed)
}
