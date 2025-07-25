## -*-python-*-
## dueca.cnf: created with DUECA version @dueca-version@
## Created on: @date@

### import the dueca namespace; provided by the c++ code
import dueca
import os
import sys
sys.path.append(os.getcwd())
from which_dueca_gtk import which_dueca_gtk

### parameters defining cooperation with other nodes
this_node_id = 3                       # id of the current node
no_of_nodes = 4                        # total number of nodes used
send_order = 3                         # order/prio in send cycle

### parameter defining real_time behaviour
highest_manager = 4                    # max priority of activities
run_in_multiple_threads = True         # test with False for threading problems
rt_sync_mode = 2                       # 0=sigwait, obsolete
                                       # 1=select, portable, obsolete
                                       # 2=nanosleep, good for all modern
                                       #   Linux kernels,
                                       #   slaves as well as masters
                                       # 3=rtc, obsolete

# graphic interface selection, typically "none", "gtk2", "gtk3", "gtk4"
graphic_interface = which_dueca_gtk()  # (automatic) selection of interface
print("Graphic interface detected as", graphic_interface)

### parameters defining "size" of the time. Note that all nodes should have
### the same compatible_increment, and for all nodes
### tick_time_step/tick_base_increment should be the same
tick_base_increment = 100              # logical increment of time, each tick
tick_compatible_increment = 100        # same, but used at start_up
tick_time_step = 0.01                  # time step for each tick
communication_interval = 100           # interval for initiating comm

### parameter for communication using multicast
if_address = "127.0.0.1"               # address of own ip interface
mc_address = "127.0.0.1"               # multicast address
mc_port = 7500                         # master control port
master_host = "127.0.0.1"              # hostname or IP of the comm master

### parameters for udp/websocket based communication (new style)
### * on the comm master (send-order 0), adjust the config url to a websocket
###   on the current machine, on a peer (send-order > 0), adjust it to point to
###   the master's websocket
config_url = "ws://" + master_host + ":" + str(mc_port) + "/config"
### * data communication, only used on comm master
###   If using udp, the network address implies the communication type;
###   multicast, broadcast, or else peer-to-peer
###   If using websockets (less real-time, better for crossing firewalls),
###   the websocket base URL must match the configuration socket
data_url = "udp://" + mc_address + ":" + str(mc_port+1)

### common communication parameters
packet_size = 8192                     # size of packets
bulk_max_size = 128*1024               # max size of bulk messages
comm_prio_level = 3                    # priority communication process
unpack_prio_level = 2                  # priority unpacking incoming data
bulk_unpack_prio_level = 2             # priority unpacking bulk data

### choice for the communication.
use_ip_comm = no_of_nodes > 1          # if true, use ethernet
classic_ip = False                     # if true, use classic ip accessor

### ___________________________________________________________________

###  1 _ ObjectManager. This enables named objects to be created,
###      and allows query of the node id and number of nodes
DUECA_objectmanager = dueca.ObjectManager(
    this_node_id, no_of_nodes).complete()

###  2 _ the environment. The environment will create the necessary
###      number of activity managers, so activities may now be
###      scheduled. From this point on it is also possible to create
###      activities
DUECA_environment = dueca.Environment().param(
    multi_thread = run_in_multiple_threads,
    highest_priority = highest_manager,
    graphic_interface = graphic_interface,
    command_interval = 0.25,
    command_lead = 0.25).complete()

### 2c _ now priority specs can be made
comm_prio = dueca.PrioritySpec(comm_prio_level, 0)
unpack_prio = dueca.PrioritySpec(unpack_prio_level, 0)
bulk_unpack_prio = dueca.PrioritySpec(bulk_unpack_prio_level, 0)

###  3 _ Packers, and a packer manager. Packers are passive
###      objects, accessed by the channels, and provide the configuration
###      data for remote communication. The unpackers use an
###         activity, and therefore must start after the environment
if use_ip_comm:
    DUECA_packer = dueca.Packer().complete()
    DUECA_unpacker = dueca.Unpacker().param(
        priority_spec = unpack_prio).complete()
    DUECA_fillpacker = dueca.FillPacker().param(
        buffer_size = bulk_max_size).complete()
    DUECA_fillunpacker = dueca.FillUnpacker().param(
        priority_spec = bulk_unpack_prio,
        buffer_size = bulk_max_size).complete()

### the packer manager keeps an inventory of all packers for transport to
### other nodes. The three arguments are a fill (bulk) packer, a normal packer
### and (if possible) a high_priority packer. One set per destination, here
### all referring to the same two packers
DUECA_packermanager = dueca.PackerManager()
if use_ip_comm:
    for i in range(no_of_nodes):
        DUECA_packermanager.param(
            add_set = dueca.PackerSet(
                DUECA_fillpacker, DUECA_packer, DUECA_packer).complete())
    DUECA_packermanager.complete()

###  4 _ The channel manager. From now on channel_using objects can
###      be created.
DUECA_channelmanager = dueca.ChannelManager().complete()

###  5 _ The ticker. A channel_using object! From now on
###      ticker_using objects can be created
DUECA_ticker = dueca.Ticker().param(
    base_increment = tick_base_increment,
    compatible_increment = tick_compatible_increment,
    time_step = tick_time_step,
    sync_mode = rt_sync_mode).complete()

###  6 _ communication hardware accessors. These may use the ticker
###      or channels to trigger activity.

if use_ip_comm and classic_ip:

    # classic IP communication
    DUECA_netcomm = dueca.IPMulticastAccessor().param(
        packer = DUECA_packer,
        unpacker = DUECA_unpacker,
        fill_packer = DUECA_fillpacker,
        fill_unpacker = DUECA_fillunpacker,
        output_buffer_size = packet_size,
        no_output_buffers = 5,
        input_buffer_size = packet_size,
        no_input_buffers = 50,
        mc_address = mc_address,
        port = mc_port,
        if_address = if_address,
        timeout = 50000,
        n_senders = no_of_nodes,
        send_order = send_order,
        time_spec = dueca.TimeSpec(0, communication_interval),
        priority = dueca.PrioritySpec(comm_prio_level, 0),
        delay_estimator = dueca.TransportDelayEstimator().param(
            const_delay = 50.1,
            delay_per_byte = 1.1,
            s_v = 20.1,
            s_const_delay = 10.1,
            s_delay_per_byte = 0.1,
            innov_max = 100.1).complete()).complete()

elif use_ip_comm and send_order == 0:

    # new UDP communication, for the send master

    # create master communicator
    DUECA_netcomm = dueca.NetMaster().param(
        packer = DUECA_packer,               # packer
        unpacker = DUECA_unpacker,           # unpacker
        fill_packer = DUECA_fillpacker,      # packer for bulk data
        fill_unpacker = DUECA_fillunpacker,  # unpacker bulk
        data_url = data_url,                 # data communication; udp
                                             # point-to-point, broadcast or
                                             # multicast (decoded from address)
                                             # or websocket
        config_url = config_url,             # configuration communication
                                             # websocket URL
        timeout = 0.2,                       # timeout, for recovery from
                                             # missing messages in data comm
        packet_size = packet_size,           # max data packet size, buffer size
        set_priority = dueca.PrioritySpec(comm_prio_level, 0),
                                             # comm priority level
        set_timing = dueca.TimeSpec(0, communication_interval),
                                             # timing cycle
        # node_list = peerlist               # comm order as a list with
        #                                    # node id's (optional)
        if_address = if_address              # network interface
    ).complete()

elif use_ip_comm and send_order != 0:

    # new UDP communication, for the peer

    # create peer communicator
    DUECA_netcomm = dueca.NetPeer().param(
        packer = DUECA_packer,               # packer
        unpacker = DUECA_unpacker,           # unpacker
        fill_packer = DUECA_fillpacker,      # packer for bulk data
        fill_unpacker = DUECA_fillunpacker,  # unpacker bulk
        config_url = config_url,             # configuration communication
                                             # websocket URL
        set_priority = dueca.PrioritySpec(comm_prio_level, 0),
                                             # comm priority level
        set_timing = dueca.TimeSpec(0, communication_interval),
                                             # timing cycle, only initial start
                                             # when not yet multi-thread
        if_address = if_address              # network interface
    ).complete()

###  7   Pass control to the environment again.
###      It will now invoke a completeCreation method
###      from the previously created singletons (1, 3, 4, 5, 6) to
###      give these the opportunity to do additional initialisation
###      Then it creates the EntityManager for this node, and the
###      configuration continues with dueca_mod.py
