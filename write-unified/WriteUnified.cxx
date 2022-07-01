/* ------------------------------------------------------------------   */
/*      item            : WriteUnified.cxx
        made by         : repa
        from template   : DusimeModuleTemplate.cxx
        template made by: Rene van Paassen
        date            : Mon Nov 15 17:20:23 2004
        category        : body file
        description     :
        changes         : Mon Nov 15 17:20:23 2004 first version
        template changes: 030401 RvP Added template creation comment
        language        : C++
*/



#define WriteUnified_cxx
// include the definition of the module class
#include "WriteUnified.hxx"

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// include additional files needed for your calculation here
#include <randNormal.hxx>
#include <CommObjectWriter.hxx>
#include <CommObjectElementWriter.hxx>
#include <boost/any.hpp>
#include <dueca/DCOtoJSON.hxx>
#include <dueca/JSONtoDCO.hxx>
// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dusime.h>

// class/module name
const char* const WriteUnified::classname = "write-unified";

// initial condition/trim table
const IncoTable* WriteUnified::getMyIncoTable()
{
  static IncoTable inco_table[] = {
    // enter pairs of IncoVariable and VarProbe pointers (i.e.
    // objects made with new), in this table.
    // For example
//    {(new IncoVariable("example", 0.0, 1.0, 0.01))
//     ->forMode(FlightPath, Constraint)
//     ->forMode(Speed, Control),
//     new VarProbe<WriteUnified,double>
//       (REF_MEMBER(&WriteUnified::i_example))}

    // always close off with:
    { NULL, NULL} };

  return inco_table;
}

// parameters to be inserted
const ParameterTable* WriteUnified::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<WriteUnified,TimeSpec>
        (&WriteUnified::setTimeSpec), set_timing_description },

    { "check-timing",
      new MemberCall<WriteUnified,vector<int> >
      (&WriteUnified::checkTiming), check_timing_description },

    { "add-blip",
      new MemberCall<WriteUnified,vstring>
      (&WriteUnified::addBlip),
      "add a new blip to the space of blips" },

    { "add-event-blip",
      new MemberCall<WriteUnified,vstring>
      (&WriteUnified::addEventBlip),
      "add a new blip to the space of blips, event style" },

    { "add-flasher-blip",
      new MemberCall<WriteUnified,vstring>
      (&WriteUnified::addFlasherBlip),
      "add a new on-and-off blip to the space of blips" },

    { "place-blip",
      new MemberCall<WriteUnified,vector<float> >
      (&WriteUnified::placeBlip),
      "put the newly added blip in its place" },

    { "place-flasher-blip",
      new MemberCall<WriteUnified,vector<float> >
      (&WriteUnified::placeFlasherBlip),
      "put the newly added blip in its place" },

    /* You can extend this table with labels and MemberCall or
       VarProbe pointers to perform calls or insert values into your
       class objects. Please also add a description (c-style string).

       Note that for efficiency, set_timing_description and
       check_timing_description are pointers to pre-defined strings,
       you can simply enter the descriptive strings in the table. */

    /* The table is closed off with NULL pointers for the variable
       name and MemberCall/VarProbe object. The description is used to
       give an overall description of the module. */
    { NULL, NULL,
      "This is a testing module for the UnifiedChannel facility."
      "It creates blips, and moves these with a random walk process."} };

  return parameter_table;
}

WriteUnified::FlasherBlipSpec::FlasherBlipSpec(const MyBlip& b) :
  b(b),
  period(100),
  countdown(period),
  token(NULL)
{
  //
}

bool WriteUnified::FlasherBlipSpec::flash()
{
  if (! (--countdown)) {
    countdown = period;
    return true;
  }
  return false;
}

ChannelWriteToken*& WriteUnified::FlasherBlipSpec::getToken()
{
  return token;
}

MyBlip& WriteUnified::FlasherBlipSpec::getBlip()
{
  return b;
}

void WriteUnified::FlasherBlipSpec::setPeriod(unsigned int period)
{
  this->period = period;
  countdown = period;
}

// constructor
WriteUnified::WriteUnified(Entity* e, const char* part, const
                       PrioritySpec& ps) :
  /* The following line initialises the SimulationModule base class.
     You always pass the pointer to the entity, give the classname and the
     part arguments.
     If you give a NULL pointer instead of the inco table, you will not be
     called for trim condition calculations, which is normal if you for
     example implement logging or a display.
     If you give 0 for the snapshot state, you will not be called to
     fill a snapshot, or to restore your state from a snapshot. Only
     applicable if you have no state. */
  SimulationModule(e, classname, part, getMyIncoTable(), 1),

  // initialize the data you need in your simulation
  dt(0.01),

  // initialize the data you need for the trim calculation
  // initialize the channel access tokens
  // example
  // my_token(getId(), NameSet(getEntity(), "MyData", part)),

  // activity initialization
  cb1(this, &WriteUnified::doCalculation),
  cb2(this, &WriteUnified::doSafe),
  do_calc(getId(), "write blips ", &cb1, ps),
  myclock(TimeSpec(0, 1))
{
  // do the actions you need for the simulation

  // connect the triggers for simulation
  do_calc.setTrigger(myclock);

  // connect the triggers for trim calculation. Leave this out if you
  // don not need input for trim calculation
  //trimCalculationCondition(/* fill in your trim triggering channels */);
}

bool WriteUnified::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
WriteUnified::~WriteUnified()
{
  //
}

// as an example, the setTimeSpec function
bool WriteUnified::setTimeSpec(const TimeSpec& ts)
{
  // a time span of 0 is not acceptable
  if (ts.getValiditySpan() == 0) return false;

  // specify the timespec to the activity
  do_calc.setTimeSpec(ts);

  // do whatever else you need to process this in your model
  dt = ts.getDtInSeconds();

  // return true if everything is acceptable
  return true;
}

// and the checkTiming function
bool WriteUnified::checkTiming(const vector<int>& i)
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

bool WriteUnified::addBlip(const vstring& s)
{
  if (s.size() > 63) {
    W_MOD(getId() << '/' << classname << " string size blip too long");
    return false;
  }

  MyBlip b; b.identification = s;
  bliplist.emplace_back
    (b,
     new ChannelWriteToken
     (getId(), NameSet(getEntity(), "MyBlip", getPart()),
      "MyBlip", s, Channel::Continuous,
      Channel::OneOrMoreEntries), false);
  return true;
}

bool WriteUnified::addEventBlip(const vstring& s)
{
  if (s.size() > 63) {
    W_MOD(getId() << '/' << classname << " string size blip too long");
    return false;
  }

  MyBlip b; b.identification = s;
  bliplist.emplace_back(b,
                        new ChannelWriteToken
                        (getId(), NameSet(getEntity(), "MyBlip", getPart()),
                         "MyBlip", s, Channel::Events,
                         Channel::OneOrMoreEntries), true);
  return true;
}


bool WriteUnified::placeBlip(const vector<float>& xyuv)
{
  if (xyuv.size() != 2 && xyuv.size() != 4) {
    E_MOD(getId() << '/' << classname << " enter 2 (pos) or 4 (pos+vel) floats");
    return false;
  }
  if (!bliplist.size()) {
    E_MOD(getId() << '/' << classname << " create a blip first");
    return false;
  }

  bliplist.back().b.x = xyuv[0];
  bliplist.back().b.y = xyuv[1];
  if (xyuv.size() == 4) {
    bliplist.back().b.dx = xyuv[2];
    bliplist.back().b.dy = xyuv[3];
  }
  else {
    bliplist.back().b.dx = 0.0;
    bliplist.back().b.dy = 0.0;
  }

  return true;
}

bool WriteUnified::addFlasherBlip(const vstring& s)
{
  if (s.size() > 63) {
    W_MOD(getId() << '/' << classname << " string size blip too long");
    return false;
  }

  MyBlip b; b.identification = s;
  flasherlist.push_back
    (FlasherBlipSpec(b));
  return true;
}

bool WriteUnified::placeFlasherBlip(const vector<float>& xyuv)
{
  if (xyuv.size() != 3 && xyuv.size() != 5) {
    E_MOD(getId() << '/' << classname << " enter 3 (pos) or 5 (pos+vel) floats");
    return false;
  }
  if (xyuv[0] < 5.0f) {
    E_MOD(getId() << '/' << classname << " minimum period 5");
    return false;
  }
  if (!flasherlist.size()) {
    E_MOD(getId() << '/' << classname << " create a blip first");
    return false;
  }

  flasherlist.back().setPeriod(unsigned(xyuv[0]));
  flasherlist.back().getBlip().x = xyuv[1];
  flasherlist.back().getBlip().y = xyuv[2];
  if (xyuv.size() == 5) {
    flasherlist.back().getBlip().dx = xyuv[3];
    flasherlist.back().getBlip().dy = xyuv[4];
  }
  else {
    flasherlist.back().getBlip().dx = 0.0;
    flasherlist.back().getBlip().dy = 0.0;
  }

  return true;
}

// tell DUECA you are prepared
bool WriteUnified::isPrepared()
{
  // do whatever additional calculations you need to prepare the model.

  // Check that all conditions for running are good.
  // It helps to indicate what the problems are
  bool res = true;
  //  unsigned idx=0;
  for (BlipList::iterator ii = bliplist.begin();
       ii != bliplist.end(); ii++) {
    if (!ii->token->isValid()) {
      W_MOD(getId() << '/' << classname << " token not valid ");
      res = false;
    }
    /* ii->drive_recorder.complete(getEntity(),
                                getNameSet().name + std::string("blip#") +
                                boost::lexical_cast<std::string>(idx),
                                getclassname<BlipDrive>());
    if (!ii->drive_recorder.isValid()) {
      W_MOD("recorder for blip " << idx << " not yet valid");
      res = false;
      } 
      idx++; */
  }

  cout << "isPrepared " << res << endl;
  // return result of check
  return res;
}
bool WriteUnified::isInitialPrepared()
{
  // do whatever additional calculations you need to prepare the model.

  cout << "isInitialPrepared "<< endl;
  // return result of check
  return true;
}

// start the module
void WriteUnified::startModule(const TimeSpec &time)
{
  cout << "start work at " << time << endl;
  do_calc.switchOn(time);
}

// stop the module
void WriteUnified::stopModule(const TimeSpec &time)
{
  cout << "stop work at " << time << endl;
  do_calc.switchOff(time);
}

void WriteUnified::initialStartModule(const TimeSpec &time)
{
  cout << "initial start at " << time << endl;
}

// stop the module
void WriteUnified::finalStopModule(const TimeSpec &time)
{
  cout << "final stop at " << time << endl;
}

// fill a snapshot with state data. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void WriteUnified::fillSnapshot(const TimeSpec& ts,
                              Snapshot& snap, bool from_trim)
{
  // The most efficient way of filling a snapshot is with an AmorphStore
  // object.
  AmorphStore s(snap.accessData(), snap.getDataSize());

  if (from_trim) {
    // use packData(s, trim_state_variable1); ... to pack your state into
    // the snapshot
  }
  else {
    snap.data = snapdata;
    snap.coding = Snapshot::JSON;
  }
}

// reload from a snapshot. You may remove this method (and the
// declaration) if you specified to the SimulationModule that the size of
// state snapshots is zero
void WriteUnified::loadSnapshot(const TimeSpec& t, const Snapshot& snap)
{
  rapidjson::GenericDocument<rapidjson::UTF8<> > doc;
  rapidjson::ParseResult res = doc.Parse(snap.data.c_str());

  auto blpit = bliplist.begin();
  for (JValue::ConstValueIterator it = doc.Begin();
       it != doc.End(); ++it) {
    json_to_dco(*it, blpit->b);
    blpit++;
  }
  // use unPackData(s, real_state_variable1 ); ... to unpack the data
  // from the snapshot.
  // You can safely do this, while snapshot loading is going on the
  // simulation is in HoldCurrent or the activity is stopped.
}

void WriteUnified::doSafe(const TimeSpec& ts)
{
  static bool report = true;
  if (report) {
    cout << "first entry, time at" << ts << endl;
    report = false;
  }
  if (do_calc.firstCycle(ts)) {
    cout << "writer: first cycle " << ts << endl;
  }
  if (do_calc.lastCycle(ts)) {
    cout << "writer: last cycle " << ts << endl;
  }
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void WriteUnified::doCalculation(const TimeSpec& ts)
{

  // check the state we are supposed to be in
  switch (getAndCheckState(ts)) {
  case SimulationState::HoldCurrent: {
    // no updates

    break;
    }

  case SimulationState::Replay: 
  case SimulationState::Advance: {

    if (getCurrentState() == SimulationState::Advance) {
      for (BlipList::iterator ii = bliplist.begin();
           ii != bliplist.end(); ii++) {
        BlipDrive bd;
        bd.doRandom();
        ii->b.dx += bd.rx;
        ii->b.dy += bd.ry;
        ii->b.x += ii->b.dx * dt;
        ii->b.y += ii->b.dy * dt;
        //ii->drive_recorder.record(ts, bd);
      }
    }
    else {
      for (BlipList::iterator ii = bliplist.begin();
           ii != bliplist.end(); ii++) {
        BlipDrive bd;
        //ii->drive_recorder.replay(ts, bd);
        ii->b.dx += bd.rx;
        ii->b.dy += bd.ry;
        ii->b.x += ii->b.dx * dt;
        ii->b.y += ii->b.dy * dt;
      }
    }

    // now the flasher blips (not controlled replay/advance diff)
    for (FlasherList::iterator ii = flasherlist.begin();
         ii != flasherlist.end(); ii++) {

      // delete or create tokens?
      if (ii->flash()) {
        if (ii->getToken() == NULL) {
          cout << "creating new token" << endl;
          ii->getToken() = new ChannelWriteToken
            (getId(), NameSet(getEntity(), "MyBlip", getPart()),
             "MyBlip", "flash",
             Channel::Continuous, Channel::OneOrMoreEntries);
        }
        else {
          cout << "deleting token" << endl;
          delete ii->getToken();
          ii->getToken() = NULL;
        }
      }

      // and move the blip
      ii->getBlip().dx += randNormal() * dt;
      ii->getBlip().dy += randNormal() * dt;
      ii->getBlip().x += ii->getBlip().dx * dt;
      ii->getBlip().y += ii->getBlip().dy * dt;
    }

    break;
    }
  default:
    // other states should never be entered for a SimulationModule,
    // HardwareModules on the other hand have more states. Throw an
    // exception if we get here,
    throw CannotHandleState(getId(),GlobalId(), "state unhandled");
  }

  // write the data
  for (BlipList::iterator ii = bliplist.begin();
       ii != bliplist.end(); ii++) {
    DataWriter<MyBlip> b(*(ii->token), ts);
    b.data() = ii->b;
  }
  
  for (FlasherList::iterator ii = flasherlist.begin();
       ii != flasherlist.end(); ii++) {
    if (ii->getToken() != NULL && ii->getToken()->isValid()) {
      DataWriter<MyBlip> b(*(ii->getToken()), ts);
      b.data() = ii->getBlip();
    }
  }

  // DUECA applications are data-driven. From the time a module is switched
  // on, it should produce data, so that modules "downstreams" are
  // activated
  // access your output channel(s)
  // example
  // StreamWriter<MyOutput> y(output_token, ts);

  // write the output into the output channel, using the stream writer
  // y.data().var1 = something; ...

  if (snapshotNow()) {
    smartstring::json_string_writer wrtr(snapdata);
    wrtr.StartArray();
    for (auto const &blp: bliplist) {
      dco_to_json(wrtr, blp.b);
    }
    wrtr.EndArray();

    // keep a copy of the model state. Snapshot sending is done in the
    // sendSnapshot routine, later, and possibly at lower priority
    // e.g.
    // snapshot_state_variable1 = state_variable1; ...
    // (or maybe if your state is very large, there is a cleverer way ...)
  }
}

void WriteUnified::trimCalculation(const TimeSpec& ts, const TrimMode& mode)
{
  // read the event equivalent of the input data
  // example
  // EventReader u(i_input_token, ts);

  // using the input, and the data put into your trim variables,
  // calculate the derivative of the state. DO NOT use the state
  // vector of the normal simulation here, because it might be that
  // this is done while the simulation runs!
  // Some elements in this state derivative are needed as target, copy
  // these out again into trim variables (see you TrimTable

  // trim calculation
  switch(mode) {
  case FlightPath: {
    // one type of trim calculation, find a power setting and attitude
    // belonging to a flight path angle and speed
  }
  break;

  case Speed: {
    // find a flightpath belonging to a speed and power setting (also
    // nice for gliders)
  }
  break;

  case Ground: {
    // find an altitude/attitude belonging to standing still on the
    // ground, power/speed 0
  }
  break;

  default:
    W_MOD(getId() << " cannot calculate inco mode " << mode);
  break;
  }

  // This works just like a normal calculation, only you provide the
  // steady state value (if your system is stable anyhow). So, if you
  // have other modules normally depending on your output, you should
  // also produce the equivalent output here.
  // EventWriter<MyOutput> y(output_token, ts);

  // write the output into the output channel, using the EventWriter
  // N.B. you may also use:
  // output_token.sendEvent(ts, new MyOutput(arguments));
  // this is slightly more efficient. However, the EventWriter makes
  // it easier, because the code will look much more like your normal
  // calculation code.

  // now return. The real results from the trim calculation, as you
  // specified them in the TrimTable, will now be collected and sent
  // off for processing.
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the scheme-interpreting code, and enable the
// creation of modules of this type
static TypeCreator<WriteUnified> a(WriteUnified::getMyParameterTable());
