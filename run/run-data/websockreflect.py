#!/usr/bin/python3

import asyncio
import websockets
import json
import msgpack


""" Test the WebSocketsServer module for DUECA

Possible url's offered by the server:

* /configuration
  overview of the configured channels

* /read/<name>?entry=n
  read JSON with a single time tick and single set of data

* /current/<name>?entry=n
  for each message, get a JSON with current data

* /info/<name>
  get a cookie for channel info changes

* /write/<name>
  write entry

* /write-and-read/<name>
  write and read server configuration


This test uses the following, on port 8001 for JSON, port 8002 for msgpack:

* Communication with a test server that maintains a pair of
  connections (in the form of channel entries) for each websocket client.
  This uses connection:

  - 172.0.0.1:{port}/write-and-read/server

  The write-and-read connection is configured to sends a SimpleCounter
  "to" a DUECA server module, on channel:

  - SimpleCounter://{entity_name}/upstream

  And send a SimpleCounter to the web sockets, from channel:

  - SimpleCounter://{entity_name}/downstream

  As the connection is made, a message with the desired DCO type
  must be sent by the client. The first reply contains information on
  the communication of the the websocket link. The test server will
  then send a series of messages, to each client individually, and
  expect a reply from the client on each message.

* Communication with a test server that uses one channel to
  send data to, and checks the responses in the entries of a number
  of configured channels.

  The channel with data from the test server is named

  - SimpleCounter://{entity_name}

  Data from this channel is presented in two different ways:

  - 127.0.0.1:{port}/read/direct
  - 127.0.0.1:{port}/current/direct

  The first one pushes all messages from the channel over the websocket
  link, the second one replies with the current value to any message
  from the link. The first message in both links describes the data
  being sent.

  Data back to the server is sent over three connections:

  - 127.0.0.1:{port}/write/directcurrent

    Multiple entries, to return the data from the /current/direct endpoint. This is
    sent over the following channel:

    - SimpleCounter://{entity_name}/from_current

  - 127.0.0.1:{port}/write/directread

    Multiple entries, to return the data from the /read/direct endpoint. This
    is sent over the following channel:

    - SimpleCounter://{entity_name}/from_read

  - 127.0.0.1:{port}/write/preset

    One preset entry per connection. This is sent over the following channels:

    - SimpleCounter://{entity_name}/preset{1,2}

    Each has a single entry only (preset, so always valid), also returns the data
    from the /read/direct endpoint.
"""

# read and decode configuration data, print it
async def configdata(decoder, port):
    url = f'ws://127.0.0.1:{port}/configuration'

    connected = False
    while not connected:
        try:
            # read task
            async with websockets.connect(url) as wsock:
                print("connection to", url)
                connected = True
                try:
                    print("waiting for data", url)
                    data = await wsock.recv()
                    info = decoder.loads(data)
                    print ("info", info)
                    await wsock.close(1000, "all data received")

                except Exception as e:
                    print(f"Configuration, exception {e}")

            # now the connection is here, run other jobs
            tasks = (
                asyncio.create_task(readwrite(decoder, port, 'server')),
                asyncio.create_task(reflect(decoder, port, 'direct')),
                asyncio.create_task(checkup(decoder, port, 'direct')),
                asyncio.create_task(topreset(decoder, port, 'direct')),
                asyncio.create_task(trackconfig(decoder, port, 'direct'))
            )
            #await readwrite(decoder, port, 'server')
            #await reflect(decoder, port, 'direct')
            await asyncio.gather(*tasks)

        except ConnectionRefusedError:
            print("No connection yet on", url)
            await asyncio.sleep(5)


async def readwrite(coder, port, endpoint):
    url = f'ws://127.0.0.1:{port}/write-and-read/{endpoint}'
    firstmsg = True

    try:

        async with websockets.connect(url) as wsock:
            print("Established write-and-read connection to", url)

            # write the requested data communication
            conf = dict(dataclass="SimpleCounter", label=f"follow{port}")
            await wsock.send(coder.dumps(conf))

            # get the first message with setup/confirmation
            msg = await wsock.recv()
            conf = coder.loads(msg)
            print("write-and-read setup", conf)

            while True:

                msg = await wsock.recv()

                data = coder.loads(msg)

                print("Replying message", data)
                res = await wsock.send(coder.dumps(data))
    except Exception as e:
        print(f"write-and-read {url}, exception {e}")

async def reflect(coder, port, endpoint):
    urlr = f'ws://127.0.0.1:{port}/read/{endpoint}'
    urlw = f'ws://127.0.0.1:{port}/write/{endpoint}read'
    firstmsg = True

    try:

        async with websockets.connect(urlr) as wsockr:

            async with websockets.connect(urlw) as wsockw:

                # send config to write socket
                conf = dict(dataclass="SimpleCounter", label="reflect")
                await wsockw.send(coder.dumps(conf))

                print("Established read connection to", urlr, urlw)

                # get the first message
                msg = await wsockr.recv()
                conf = coder.loads(msg)
                print("read setup", conf)

                msg = await wsockw.recv()
                conf = coder.loads(msg)
                print("write setup", conf)

                while True:

                    msg = await wsockr.recv()

                    data = coder.loads(msg)

                    print("Replying message", data)
                    res = await wsockw.send(coder.dumps(data))
    except Exception as e:
        print(f"reflect {urlw}, exception {e}")

async def topreset(coder, port, endpoint):
    urlr = f'ws://127.0.0.1:{port}/read/{endpoint}'
    urlw = f'ws://127.0.0.1:{port}/write/{endpoint}preset'
    firstmsg = True

    try:

        async with websockets.connect(urlr) as wsockr:

            async with websockets.connect(urlw) as wsockw:

                # send config to write socket
                conf = dict(dataclass="SimpleCounter", label="reflect")
                await wsockw.send(coder.dumps(conf))

                print("Established read connection to", urlr, urlw)

                # get the first message
                msg = await wsockr.recv()
                conf = coder.loads(msg)
                print("read setup", conf)

                msg = await wsockw.recv()
                conf = coder.loads(msg)
                print("write setup", conf)

                while True:

                    msg = await wsockr.recv()

                    data = coder.loads(msg)

                    print("Replying message", data)
                    res = await wsockw.send(coder.dumps(data))
    except Exception as e:
        print(f"reflect {urlw}, exception {e}")

async def trackconfig(coder, port, endpoint):
    urlc = f'ws://127.0.0.1:{port}/info/{endpoint}'
    try:
        async with websockets.connect(urlc) as wsockc:

            print("Established read connection to", urlc)
            while True:
                msg = await wsockc.recv()
                conf = coder.loads(msg)
                print("Channel info", urlc, conf)

    except Exception as e:
        print(f"info {urlc}, exception {e}")

async def checkup(coder, port, endpoint):
    urlr = f'ws://127.0.0.1:{port}/current/{endpoint}'
    urlw = f'ws://127.0.0.1:{port}/write/{endpoint}current'
    firstmsg = True

    try:
        async with websockets.connect(urlr) as wsockr:

            async with websockets.connect(urlw) as wsockw:

                # send config to write socket
                conf = dict(dataclass="SimpleCounter", label="reflect")
                await wsockw.send(coder.dumps(conf))

                print("Established read connection to", urlr, urlw)

                # get the first message
                msg = await wsockr.recv()
                conf = coder.loads(msg)
                print("read setup", conf)

                msg = await wsockw.recv()
                conf = coder.loads(msg)
                print("write setup", conf)

                olddata = {}
                while True:

                    await asyncio.sleep(0.1)
                    await wsockr.send(coder.dumps({}))
                    msg = await wsockr.recv()

                    data = coder.loads(msg)
                    if olddata != data:
                        print("New message", data)
                        olddata = data
                        res = await wsockw.send(coder.dumps(data))

    except Exception as e:
        print(f"checkup {urlw}, exception {e}")


async def run_jobs():
    result = await asyncio.gather(
        configdata(json, 8001),
        configdata(msgpack, 8002)
    )


try:
    loop = asyncio.get_running_loop()
except RuntimeError:
    asyncio.new_event_loop()
    loop = None

if loop:
    # probably running from spyder
    asyncio.create_task(run_jobs())

else:
    asyncio.run(run_jobs())
