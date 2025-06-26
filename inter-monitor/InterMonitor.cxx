/* ------------------------------------------------------------------   */
/*      item            : InterMonitor.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx (2022.06)
        date            : Thu Jun 26 09:50:09 2025
        category        : body file
        description     :
        changes         : Thu Jun 26 09:50:09 2025 first version
        language        : C++
        copyright       : (c)
*/

#define InterMonitor_cxx

// include the definition of the module class
#include "InterMonitor.hxx"

// include additional files needed for your calculation here
#include <dueca/inter/ReplicatorInfo.hxx>
#include <dueca/inter/ReplicatorPeerAcknowledge.hxx>
#include <dueca/inter/ReplicatorPeerJoined.hxx>

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>

// include the debug writing header, by default, write warning and
// error messages
#define I_MOD
#include <debug.h>

// class/module name
const char *const InterMonitor::classname = "inter-monitor";

// Parameters to be inserted
const ParameterTable *InterMonitor::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

    { "deny-access-to",
      new VarProbe<_ThisModule_, std::string>(&_ThisModule_::deny_access_to),
      "For testing, deny access to the given peer" },
    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL, "please give a description of this module" }
  };

  return parameter_table;
}

// constructor
InterMonitor::InterMonitor(Entity *e, const char *part,
                           const PrioritySpec &ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments. */
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  r_replicatorinfo(getId(),
                   NameSet(getEntity(), getclassname<ReplicatorInfo>(), ""),
                   getclassname<ReplicatorInfo>()),
  r_peernotice(getId(),
               NameSet(getEntity(), getclassname<ReplicatorPeerJoined>(), ""),
               getclassname<ReplicatorPeerJoined>()),
  w_peerack(getId(),
            NameSet(getEntity(), getclassname<ReplicatorPeerAcknowledge>(), ""),
            getclassname<ReplicatorPeerAcknowledge>(), "", Channel::Events),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "process interconnect info", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(r_peernotice || r_replicatorinfo);
}

bool InterMonitor::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
InterMonitor::~InterMonitor()
{
  //
}

// as an example, the setTimeSpec function
bool InterMonitor::setTimeSpec(const TimeSpec &ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0)
    return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);
  // or do this with the clock if you have it (don't do both!)
  // myclock.changePeriodAndOffset(ts);

  // do whatever else you need to process this in your model
  // hint: ts.getDtInSeconds()

  // return true if everything is acceptable
  return true;
}

// the checkTiming function installs a check on the activity/activities
// of the module
bool InterMonitor::checkTiming(const std::vector<int> &i)
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
bool InterMonitor::isPrepared()
{
  bool res = true;

  // Example checking a token:
  CHECK_TOKEN(r_peernotice);
  CHECK_TOKEN(r_replicatorinfo);
  CHECK_TOKEN(w_peerack);

  // return result of checks
  return res;
}

// start the module
void InterMonitor::startModule(const TimeSpec &time) { do_calc.switchOn(time); }

// stop the module
void InterMonitor::stopModule(const TimeSpec &time) { do_calc.switchOff(time); }

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void InterMonitor::doCalculation(const TimeSpec &ts)
{
  while (r_peernotice.haveVisibleSetsInEntry()) {
    DataReader<ReplicatorPeerJoined> nt(r_peernotice);
    DataWriter<ReplicatorPeerAcknowledge> ack(w_peerack, nt.timeSpec());
    ack.data().peerdata = "Welcome";
    ack.data().peer_id = nt.data().peer_id;
    I_MOD("Welcoming peer " << nt.data().peer_id);
  }

  while (r_replicatorinfo.haveVisibleSetsInEntry()) {
    DataReader<ReplicatorInfo> inf(r_replicatorinfo);
    I_MOD("Replicator info " << inf.data());
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<InterMonitor> a(InterMonitor::getMyParameterTable());
