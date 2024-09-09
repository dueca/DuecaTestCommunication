/* ------------------------------------------------------------------   */
/*      item            : ReadWriteServer.hxx
        made by         : repa
        from template   : DusimeModuleTemplate.hxx (2022.06)
        date            : Fri Sep  6 17:24:12 2024
        category        : header file
        description     :
        changes         : Fri Sep  6 17:24:12 2024 first version
        language        : C++
        copyright       : (c)
*/

#ifndef ReadWriteServer_hxx
#define ReadWriteServer_hxx

// include the dusime header
#include "DataTimeSpec.hxx"
#include "GlobalId.hxx"
#include <boost/smart_ptr/scoped_ptr.hpp>
#include <dueca/ChannelWatcher.hxx>
#include <dusime.h>
#include <list>

USING_DUECA_NS;

// This includes headers for the objects that are sent over the channels
#include "comm-objects.h"

// include headers for functions/classes you need in the module

/** A module.

    The instructions to create an module of this class from the start
    script are:

    \verbinclude read-write-server.scm
*/
class ReadWriteServer : public SimulationModule
{
  /** self-define the module type, to ease writing the parameter table */
  typedef ReadWriteServer _ThisModule_;

private: // simulation data
  // declare the data you need in your simulation

  /** Communication client data */
  struct CommClient
  {
    /** State machine definition */
    enum Phase {
      CheckTokens,  ///< Check the read and write token
      Counting,     ///< Writing and checking responses
      WaitResponse, ///< Waiting for readback
      Closing       ///< Waiting for removal
    };

    /** Current phase */
    Phase phase;

    /** Current value of this client's counter */
    int counter;

    /** Read token on the communication channel */
    ChannelReadToken r_token;

    /** Reply/ write token on the channel */
    boost::scoped_ptr<ChannelWriteToken> w_token;

    /** token label */
    std::string label;

    /** Constructor */
    CommClient(const GlobalId &master_id, int ncycles, const std::string &rchannelname,
               const std::string &wchannelname, unsigned entry_id,
               const std::string &label);

    /** Action, check up on responses, and send data to the client */
    bool process(const DataTimeSpec &ts);
  };

  /** List of clients */
  std::list<CommClient> clients;

  /** Channel to watch */
  std::string readchannel_name;

  /** Channel to write to */
  std::string writechannel_name;

  /** Number of cycles */
  int ncycles;

  /** Monitor the read channel */
  boost::scoped_ptr<ChannelWatcher> watcher;

  /** Total number of clients processed */
  unsigned totalclients;

private: // trim calculation data
  // declare the trim calculation data needed for your simulation
private: // snapshot data
  // declare, if you need, the room for placing snapshot data
private: // channel access
  // declare access tokens for all the channels you read and write
  // examples:
  // ChannelReadToken    r_mytoken;
  // ChannelWriteToken   w_mytoken;
private: // activity allocation
  /** You might also need a clock. Don't mis-use this, because it is
      generally better to trigger on the incoming channels */
  PeriodicAlarm myclock;

  /** Callback object for simulation calculation. */
  Callback<ReadWriteServer> cb1;

  /** Activity for simulation calculation. */
  ActivityCallback do_calc;

public: // class name and trim/parameter tables
  /** Name of the module. */
  static const char *const classname;

  /** Return the initial condition table. */
  static const IncoTable *getMyIncoTable();

  /** Return the parameter table. */
  static const ParameterTable *getMyParameterTable();

public: // construction and further specification
  /** Constructor. Is normally called from the creation script. */
  ReadWriteServer(Entity *e, const char *part, const PrioritySpec &ts);

  /** Continued construction. This is called after all script
      parameters have been read and filled in, according to the
      parameter table. Your running environment, e.g. for OpenGL
      drawing, is also prepared. Any lengthy initialisations (like
      reading the 4 GB of wind tables) should be done here.
      Return false if something in the parameters is wrong (by
      the way, it would help if you printed what!) May be deleted. */
  bool complete();

  /** Destructor. */
  ~ReadWriteServer();

  // add here the member functions you want to be called with further
  // parameters. These are then also added in the parameter table
  // The most common one (addition of time spec) is given here.
  // Delete if not needed!

  /** Specify a time specification for the simulation activity. */
  bool setTimeSpec(const TimeSpec &ts);

  /** Request check on the timing. */
  bool checkTiming(const std::vector<int> &i);

public: // member functions for cooperation with DUECA
  /** indicate that everything is ready. */
  bool isPrepared();

  /** start responsiveness to input data. */
  void startModule(const TimeSpec &time);

  /** stop responsiveness to input data. */
  void stopModule(const TimeSpec &time);

public: // the member functions that are called for activities
  /** the method that implements the main calculation. */
  void doCalculation(const TimeSpec &ts);
};

#endif
