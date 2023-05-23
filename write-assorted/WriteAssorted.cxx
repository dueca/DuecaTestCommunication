/* ------------------------------------------------------------------   */
/*      item            : WriteAssorted.cxx
        made by         : repa
        from template   : DuecaModuleTemplate.cxx (2022.06)
        date            : Mon May 22 15:08:17 2023
        category        : body file
        description     :
        changes         : Mon May 22 15:08:17 2023 first version
        language        : C++
        copyright       : (c)
*/

#define WriteAssorted_cxx

// include the definition of the module class
#include "WriteAssorted.hxx"

// include additional files needed for your calculation here

// the standard package for DUSIME, including template source
#define DO_INSTANTIATE
#include <dueca.h>

// include the debug writing header, by default, write warning and
// error messages
#define W_MOD
#define E_MOD
#include <debug.h>

// class/module name
const char *const WriteAssorted::classname = "write-assorted";

// Parameters to be inserted
const ParameterTable *WriteAssorted::getMyParameterTable()
{
  static const ParameterTable parameter_table[] = {
    { "set-timing",
      new MemberCall<_ThisModule_, TimeSpec>(&_ThisModule_::setTimeSpec),
      set_timing_description },

    { "check-timing",
      new MemberCall<_ThisModule_, std::vector<int>>(
        &_ThisModule_::checkTiming),
      check_timing_description },

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
WriteAssorted::WriteAssorted(Entity *e, const char *part,
                             const PrioritySpec &ps) :
  Module(e, classname, part),

  // initialize the data you need in your simulation or process
  count(0),

  // initialize the channel access tokens, check the documentation for the
  // various parameters. Some examples:
  w_blipchild(getId(), NameSet(getEntity(), getclassname<BlipChild>(), part),
              getclassname<BlipChild>(), "child", Channel::Events),
  w_blipdrive(getId(), NameSet(getEntity(), getclassname<BlipDrive>(), part),
              getclassname<BlipDrive>(), "drive", Channel::Events),

  w_testfixvector(getId(),
                  NameSet(getEntity(), getclassname<TestFixVector>(), part),
                  getclassname<TestFixVector>(), "fix", Channel::Events),

  w_testlimvector(getId(),
                  NameSet(getEntity(), getclassname<TestLimVector>(), part),
                  getclassname<TestLimVector>(), "lim", Channel::Events),
  w_testvarvector(getId(),
                  NameSet(getEntity(), getclassname<TestVarVector>(), part),
                  getclassname<TestVarVector>(), "var", Channel::Events),
  w_testlists(getId(), NameSet(getEntity(), getclassname<TestLists>(), part),
              getclassname<TestLists>(), "lists", Channel::Events),

  w_testmap(getId(), NameSet(getEntity(), getclassname<TestMap>(), part),
            getclassname<TestMap>(), "map", Channel::Events),

  w_testnestedmap(getId(),
                  NameSet(getEntity(), getclassname<TestNestedMap>(), part),
                  getclassname<TestNestedMap>(), "nest", Channel::Events),
  w_xmlcoded(getId(), NameSet(getEntity(), getclassname<XMLCoded>(), part),
             getclassname<XMLCoded>(), "xml", Channel::Events),

  w_testmappedfixvector(
    getId(), NameSet(getEntity(), getclassname<TestMappedFixVector>(), part),
    getclassname<TestMappedFixVector>(), "mapvec", Channel::Events),

  myclock(),

  // a callback object, pointing to the main calculation function
  cb1(this, &_ThisModule_::doCalculation),
  // the module's main activity
  do_calc(getId(), "write all kinds of dco data", &cb1, ps)
{
  // connect the triggers for simulation
  do_calc.setTrigger(myclock);
}

bool WriteAssorted::complete()
{
  /* All your parameters have been set. You may do extended
     initialisation here. Return false if something is wrong. */
  return true;
}

// destructor
WriteAssorted::~WriteAssorted()
{
  //
}

// as an example, the setTimeSpec function
bool WriteAssorted::setTimeSpec(const TimeSpec &ts)
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

// the checkTiming function installs a check on the activity/activities
// of the module
bool WriteAssorted::checkTiming(const std::vector<int> &i)
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
bool WriteAssorted::isPrepared()
{
  bool res = true;

  // check tokens:
  CHECK_TOKEN(w_blipchild);
  CHECK_TOKEN(w_blipdrive);
  CHECK_TOKEN(w_testfixvector);
  CHECK_TOKEN(w_testlimvector);
  CHECK_TOKEN(w_testvarvector);
  CHECK_TOKEN(w_testlists);
  CHECK_TOKEN(w_testmap);
  CHECK_TOKEN(w_testnestedmap);
  CHECK_TOKEN(w_xmlcoded);
  CHECK_TOKEN(w_testmappedfixvector);

  // return result of checks
  return res;
}

// start the module
void WriteAssorted::startModule(const TimeSpec &time)
{
  do_calc.switchOn(time);
}

// stop the module
void WriteAssorted::stopModule(const TimeSpec &time)
{
  do_calc.switchOff(time);
}

// this routine contains the main simulation process of your module. You
// should read the input channels here, and calculate and write the
// appropriate output
void WriteAssorted::doCalculation(const TimeSpec &ts)
{
  count++;
  {
    DataWriter<BlipChild> w(w_blipchild, ts);
    w.data().count = count;
  }
  {
    DataWriter<BlipDrive> w(w_blipdrive, ts);
    w.data().rx = count;
  }
  {
    DataWriter<TestFixVector> w(w_testfixvector, ts);
    w.data().nums[1] = count;
  }
  {
    DataWriter<TestLimVector> w(w_testlimvector, ts, count % 4 + 1);
    w.data().blips[count % 4].x = count;
  }
  {
    DataWriter<TestVarVector> w(w_testvarvector, ts, 3);
    w.data().nums[2] = count;
  }
  {
    DataWriter<TestLists> w(w_testlists, ts);
    w.data().l_ints.push_back(count);
  }
  {
    DataWriter<TestMap> w(w_testmap, ts);
    w.data().items["one"] = 1;
    w.data().items["count"] = count;
  }
  {
    DataWriter<TestNestedMap> w(w_testnestedmap, ts);
    w.data().blips["first"] = MyBlip();
    w.data().blips["second"] = MyBlip();
  }
  {
    DataWriter<XMLCoded> w(w_xmlcoded, ts);
    if (count % 2 == 0) {
      w.data().document.encodexml(MyBlip());
    }
    else {
      w.data().document.encodejson(MyBlip());
    }
  }
  {
    DataWriter<TestMappedFixVector> w(w_testmappedfixvector, ts);
    w.data().mfv["0"][0] = double(count);
    w.data().mfv["1"][1] = double(count);
  }
}

// Make a TypeCreator object for this module, the TypeCreator
// will check in with the script code, and enable the
// creation of modules of this type
static TypeCreator<WriteAssorted> a(WriteAssorted::getMyParameterTable());
