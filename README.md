## Exploiting Nil-Externality for Fast Replicated Storage - Overview

Skyros is a new replication protocol that exploits nil-externality, a property of storage interfaces. Skyros offers high performance by deferring ordering and executing operations until their effects are externalized. This repository contains Skyros' implementation and related experiments appearing in the paper titled "Exploiting Nil-Externality for Fast Replicated Storage" in SOSP 2021. 
 
If you are an artifact evaluator for SOSP 21, we recommend you to look at [./experiments/README.md](./experiments/README.md) to know more about how to run experiments and produce graphs. To make your process easier, we have set up a cluster for you in AWS EC2 and installed the necessary packages and dependencies. Please look at [./experiments/README.md](./experiments/README.md) for further information.

If you would like to run the cluster by yourself, please refer to [./src/README.md](./src/README.md). 