# DuecaTestCommunication, a DUECA Project

## Introduction

This is a test project for DUECA, stressing channel writing and reading.
It runs with [DUECA Middleware](https://github.com/dueca/dueca), do not
clone directly but use DUECA's `dueca-gproject` script for that.

## Application

This has no other application than testing DUECA communication and
triggering. The different modules test the following:

### ReadUnified and WriteUnified

Simulate a process (random walk), with "MyBlip" objects, some of which
are created at the start of the simulation, and some of which are
repeatedly created and removed again.

### WriteAssorted

Write all kinds of different DCO objects. The objects will be read by
the standard HDF5 and DDFF logging modules built into DUECA.

### CheckTriggering

Oriented at testing triggering, verifies that the different channels
trigger correctly.

### ReadWriteServer

Module to test write-and-read endpoints, write endpoints, and read endpoints
of the websocket server.

## Author(s)

René van Paassen

## LICENSE

EUPL-1.2

