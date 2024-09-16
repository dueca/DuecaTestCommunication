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
                asyncio.create_task(checkup(decoder, port, 'direct'))
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

                _olddata = {}
                while True:

                    await asyncio.timeout(0.1)
                    await sworkr.send(coder.dumps({}))
                    msg = await wsockr.recv()

                    data = coder.loads(msg)

                    if olddata != data:
                        print("New message", data)
                        olddata = data
                        res = await wsockw.send(coder.dumps(data))

    except Exception as e:
        print(f"checup {urlw}, exception {e}")


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
