#ifndef DASH_ADAPTATION_LOGIC_FACTORY
#define DASH_ADAPTATION_LOGIC_FACTORY

#define ENSURE_ADAPTATION_LOGIC_REGISTERED(x) player::AdaptationLogicFactory::RegisterAdaptationLogic(x::GetName(), &(x::Create));

#include <iostream>
#include <string>
#include <map>
#include <memory>

#include <boost/shared_ptr.hpp>

namespace player
{
class DashPlayer;
class AdaptationLogic;

typedef boost::shared_ptr<AdaptationLogic> (*ALogicFunctionPointer)(DashPlayer*);

class AdaptationLogicFactory
{
public:
  AdaptationLogicFactory() : AvailALogics()
  {
  }

  static boost::shared_ptr<AdaptationLogicFactory> GetInstance()
  {
      static boost::shared_ptr<AdaptationLogicFactory> instance = boost::shared_ptr<AdaptationLogicFactory>(new AdaptationLogicFactory());
      return instance;
  }

  /** registry of adaptation logics */
  std::map<std::string /* aLogicTypeName */, ALogicFunctionPointer /* ptr */> AvailALogics;
public:
  static
  void RegisterAdaptationLogic(std::string aLogicType,
                        ALogicFunctionPointer functionPointer)
  {
    fprintf(stderr, "registering %s\n", aLogicType.c_str());
    if (GetInstance()->AvailALogics.find(aLogicType) == GetInstance()->AvailALogics.end())
    {
      std::cerr << "AdaptationLogicFactory():\tRegistering Adaptation Logic of Type " << aLogicType << std::endl;
      GetInstance()->AvailALogics[aLogicType] = functionPointer;
    }
  }


  /** create an instance of an adaptation logic based on the provided string */
  static boost::shared_ptr<AdaptationLogic> Create(std::string adaptationLogicType, DashPlayer* mPlayer)
  {
    if (GetInstance()->AvailALogics.find(adaptationLogicType) != GetInstance()->AvailALogics.end())
    {
      // call the function pointer (Create Function), and pass the mPlayer pointer
      ALogicFunctionPointer fptr = (GetInstance()->AvailALogics[adaptationLogicType]);

      return fptr(mPlayer);
    }
    else
    {
      std::cerr << "AdaptationLogicFactory():\tError - Could not find Adaptation Logic of Type " << adaptationLogicType << std::endl;
      return boost::shared_ptr<AdaptationLogic>();
      //return nullptr;
    }
  }
};

}


#endif // DASH_ADAPTATION_LOGIC_FACTORY
