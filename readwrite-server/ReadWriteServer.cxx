/* ------------------------------------------------------------------   */
/*      item            : ReadWriteServer.cxx
        made by         : repa
        from template   : DusimeModuleTemplate.cxx (2022.06)
        date            : Fri Sep  6 17:24:12 2024
        category        : body file
        description     :
        changes         : Fri Sep  6 17:24:12 2024 first version
        language        : C++
        copyright       : (c)
*/

#include "ChannelWatcher.hxx"
#include "CommObjectTraits.hxx"
#include "SimpleCounter.hxx"
#define ReadWriteServer_cxx
// include the definition of the module class
#include "ReadWriteServer.hxx"

// include additional files needed for your calculation here
#include <algorithm>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// include the debug writing header. Warning and error messages
// are on by default, debug and info can be selected by
// uncommenting the respective defines
// #define D_MOD
// #define I_MOD
#include <debug.h>

// class/module name
const char *const ReadWriteServer::classname = "read-write-server";

// initial condition/trim table
const IncoTable *ReadWriteServer::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<_ThisModule_,double>
//       (REF_MEMBER(&_ThisModule_::i_example))}

    // always close off with:
    { NULL, NULL }
  };

  return inco_table;
}

// parameters to be inserted
const ParameterTable *ReadWriteServer::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

    {
      "read-channel",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::readchannel_name),
      "Channel to monitor for client entries",
    },

    { "write-channel",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::writechannel_name),
      "Channel name for writing client replies" },

    { "ncycles", new VarProbe<_ThisModule_, int>(&_ThisModule_::ncycles),
      "Number of cycles to count" },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "Test service for the websocket server communication." }
  };

  return parameter_table;
}

// constructor
ReadWriteServer::ReadWriteServer(Entity *e, const char *part,
                                 const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  SimulationModule(e, classname, part, getMyIncoTable(), 4),

  // initialize the data you need in your simulation
  clients(),
  readchannel_name(),
  writechannel_name(),
  ncycles(100),
  watcher(),
  totalclients(0),
  myclock(),
  cb1(this, &_ThisModule_::doCalculation),
  do_calc(getId(), "send data and check-up", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
}

bool ReadWriteServer::complete()
{
  if (!readchannel_name.size() || !writechannel_name.size()) {
    E_MOD("Specify channel names for the ReadWriteServer");
    return false;
  }
  watcher.reset(new ChannelWatcher(readchannel_name, true));

  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
ReadWriteServer::~ReadWriteServer()
{
  if (totalclients) {
    if (clients.size()) {
      for (const auto &c: clients) {
        E_MOD("Remaining uncompleted client for " << c.label << " phase " << c.phase << " count " << c.counter);
      }
      exit(1);
    }
  }
  else {
    W_MOD("No clients on " << readchannel_name);
  }
}

// as an example, the setTimeSpec function
bool ReadWriteServer::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // specify the timespec to the activity
  // do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool ReadWriteServer::checkTiming(const std::vector<int> &i)
{
  if (i.size() == 3) {
    new TimingCheck(do_calc, i[0], i[1], i[2]);
  }
  else if (i.size() == 2) {
    new TimingCheck(do_calc, i[0], i[1]);
  }
  else {
    return false;
  }
  return true;
}

// tell DUECA you are prepared
bool ReadWriteServer::isPrepared()
{
  bool res = true;

  // return result of checks
  return res;
}

// start the module
void ReadWriteServer::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void ReadWriteServer::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void ReadWriteServer::doCalculation(const TimeSpec &ts)
{
  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {
    // only repeat the output, do not change the model state

    break;
  }

  case SimulationState::Replay:
  case SimulationState::Advance: {

    for (auto &client : clients) {
      if (client.process(ts)) {
        // whatever
      }
    }

    break;
  }
  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(), GlobalId(), "state unhandled");
  }

  // check up on entries
  ChannelEntryInfo ei;
  while (watcher->checkChange(ei)) {
    if (ei.created) {
      // create a matching processor
      clients.emplace_back(getId(), ncycles, readchannel_name,
                           writechannel_name, ei.entry_id, ei.entry_label);
      totalclients++;
    }
    else {
      // remove and check
      auto cl =
        std::find_if(clients.begin(), clients.end(), [ei](const CommClient &c) {
          return c.label == ei.entry_label;
        });
      if (cl == clients.end()) {
        E_MOD("Cannot find client handler for " << ei.entry_label);
      }
      else {
        if (cl->phase != CommClient::Closing) {
          E_MOD("Client not properly closed for " << ei.entry_label);
        }
        else {
          clients.erase(cl);
        }
      }
    }
  }
}

ReadWriteServer::CommClient::CommClient(const GlobalId &master_id, int ncycles,
                                        const std::string &rchannelname,
                                        const std::string &wchannelname,
                                        unsigned entry_id,
                                        const std::string &label) :
  phase(CheckTokens),
  counter(ncycles),
  r_token(master_id, NameSet(rchannelname), getclassname<SimpleCounter>(),
          entry_id, Channel::Events, Channel::OneOrMoreEntries),
  w_token(new ChannelWriteToken(master_id, wchannelname,
                                getclassname<SimpleCounter>(), label,
                                Channel::Events, Channel::OneOrMoreEntries)),
  label(label)
{}

bool ReadWriteServer::CommClient::process(const DataTimeSpec &ts)
{
  switch (phase) {

  case CheckTokens:
    if (r_token.isValid() && w_token->isValid()) {
      phase = Counting;
    }
    break;

  case Counting:
    if (counter) {
      counter--;
      DataWriter<SimpleCounter> cnt(*w_token, ts);
      cnt.data().count = counter;
      phase = WaitResponse;
    }
    else {
      w_token.reset();
      phase = Closing;
    }
    break;

  case WaitResponse:
    if (r_token.haveVisibleSets(ts)) {
      DataReader<SimpleCounter> cnt(r_token, ts);
      if (cnt.data().count != counter) {
        W_MOD("Unexpected data read back from client at '"
              << label << "', expecting " << counter << " received "
              << cnt.data().count);
      }
      else {
        phase = Counting;
      }
    }
    break;

  case Closing:
    break;
  }
  return true;
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<ReadWriteServer> a(ReadWriteServer::getMyParameterTable());