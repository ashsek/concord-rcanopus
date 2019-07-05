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
	"os"
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

	//Read only,
	// const request_membership2 = `{"Mode": "public_keys", "BGID": "0"}`
	// const request_membership2 = `{"Mode": "quorum_size", "BGID": "0"}`
	// const request_membership2 = `{"Mode": "byzantine_groups", "BGID": "0"}`
//	 const request_membership2 = `{"Mode": "emulators", "BGID": "0", "height":"2"}`

	//For modifications,
	const request_membership2 = `{"Mode": "quorum_size", "BGID":"0", "size":"69", "quorum_cert": "cert" , "cycle":"15"}`
	// const request_membership2 = `{"Mode": "public_keys", "BGID":"0", "keys": {"10.0.2.5": "edfGTS0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCzGP8PMAQZVWeUI4g2mGliU342npkRa2yuNFIhSo58XH/WYS2UBfB76U5tIevPKZhUs3hzhURBe8eLBNh0d1LYdYToTpAmcX8MfU/zxhTfbsXwvoE4JITqaC5Z92MAP0p4LWXi24CuQYjv0ryhlft1MQ89dOHEKsdzIP8GSF+BDwIDAQAB", "10.0.2.6": "BHYGFR0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCzGP8PMAQZVWeUI4g2mGliU342npkRa2yuNFIhSo58XH/WYS2UBfB76U5tIevPKZhUs3hzhURBe8eLBNh0d1LYdYToTpAmcX8MfU/zxhTfbsXwvoE4JITqaC5Z92MAP0p4LWXi24CuQYjv0ryhlft1MQ89dOHEKsdzIP8GSF+BDwIDAQAB", "certificate_msp":"certificate"}}`
	// todo: emulators,
	// Don't know about byzantine groups who should modify them as concord does not allow dynamic configuration
	const read int32 = 0 // set 1 for readonly else set 0 fopr modifications.

	// Contact the server and print out its response.
	name := request_membership2
	read_only := read

	if len(os.Args) > 1 {
		name = os.Args[1]
		// read_only = os.Args[2]
	}

	ctx, cancel := context.WithTimeout(context.Background(), time.Second)
	defer cancel()
	r, err := c.SayHello(ctx, &pb.RequestMembership{Name: name, ReadOnly: read_only})
	elapsed := time.Since(start)
	

	if err != nil {
		log.Fatalf("could not greet: %v", err)
	}
	log.Printf("Greeting: %s", r.Message)
	log.Printf("page took %s", elapsed)
}
