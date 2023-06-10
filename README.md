# Basic-neural-network
A C++ implementation of a basic neural network with complete front propogation, but incomplete back propogation. Incomplete back propogation in the sense that it does not check how accurate the answer is and in turn change the values of the weights, but rather just sends the output back to the input nodes layer by layer.

The network is implemented in a linux enoviroment, uses fork() commands for the layers, detached threads for the nodes, and named pipes (fifo) for the inter node communication.
