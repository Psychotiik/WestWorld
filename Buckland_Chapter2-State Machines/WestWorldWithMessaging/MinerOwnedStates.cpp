#include "MinerOwnedStates.h"
#include "fsm/State.h"
#include "Miner.h"
#include "Locations.h"
#include "messaging/Telegram.h"
#include "MessageDispatcher.h"
#include "MessageTypes.h"
#include "Time/CrudeTimer.h"
#include "EntityNames.h"

#include <iostream>
using std::cout;


#ifdef TEXTOUTPUT
#include <fstream>
extern std::ofstream os;
#define cout os
#endif


//------------------------------------------------------------------------methods for EnterMineAndDigForNugget

EnterMineAndDigForNugget* EnterMineAndDigForNugget::Instance()
{
  static EnterMineAndDigForNugget instance;

  return &instance;
}

void EnterMineAndDigForNugget::Enter(Miner* pMiner)
{
  //if the miner is not already located at the goldmine, he must
  //change location to the gold mine
  if (pMiner->Location() != goldmine)
  {
    cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Walkin' to the goldmine";

    pMiner->ChangeLocation(goldmine);
  }
}

void EnterMineAndDigForNugget::Execute(Miner* pMiner)
{  
  //Now the miner is at the goldmine he digs for gold until he
  //is carrying in excess of MaxNuggets. If he gets thirsty during
  //his digging he packs up work for a while and changes state to
  //gp to the saloon for a Whiskey.
  pMiner->AddToGoldCarried(1);

  pMiner->IncreaseFatigue();

  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Pickin' up a nugget";

  //if enough gold mined, go and put it in the bank
  if (pMiner->PocketsFull())
  {
    pMiner->GetFSM()->ChangeState(VisitBankAndDepositGold::Instance());
  }

  if (pMiner->Thirsty())
  {
    pMiner->GetFSM()->ChangeState(QuenchThirst::Instance());
  }
}

void EnterMineAndDigForNugget::Exit(Miner* pMiner)
{
  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " 
       << "Ah'm leavin' the goldmine with mah pockets full o' sweet gold";
}

bool EnterMineAndDigForNugget::OnMessage(Miner* pMiner, const Telegram& msg)
{
  //send msg to global message handler
  return false;
}



//------------------------------------------------------------------------methods for VisitBankAndDepositGold

VisitBankAndDepositGold* VisitBankAndDepositGold::Instance()
{
  static VisitBankAndDepositGold instance;

  return &instance;
}

void VisitBankAndDepositGold::Enter(Miner* pMiner)
{  
  //on entry the miner makes sure he is located at the bank
  if (pMiner->Location() != bank)
  {
    cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Goin' to the bank. Yes siree";

    pMiner->ChangeLocation(bank);
  }
}

void VisitBankAndDepositGold::Execute(Miner* pMiner)
{
  //deposit the gold
  pMiner->AddToWealth(pMiner->GoldCarried());
    
  pMiner->SetGoldCarried(0);

  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " 
       << "Depositing gold. Total savings now: "<< pMiner->Wealth();

  //wealthy enough to have a well earned rest?
  if (pMiner->Wealth() >= pMiner->GetComfortLevel())
  {
    cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " 
         << "WooHoo! Rich enough for now. Back home to mah li'lle lady";
      
    pMiner->GetFSM()->ChangeState(GoHomeAndSleepTilRested::Instance());      
  }

  //otherwise get more gold
  else 
  {
    pMiner->GetFSM()->ChangeState(EnterMineAndDigForNugget::Instance());
  }
}

void VisitBankAndDepositGold::Exit(Miner* pMiner)
{
  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Leavin' the bank";
}

bool VisitBankAndDepositGold::OnMessage(Miner* pMiner, const Telegram& msg)
{
  //send msg to global message handler
  return false;
}



//------------------------------------------------------------------------methods for GoHomeAndSleepTilRested

GoHomeAndSleepTilRested* GoHomeAndSleepTilRested::Instance()
{
  static GoHomeAndSleepTilRested instance;

  return &instance;
}

void GoHomeAndSleepTilRested::Enter(Miner* pMiner)
{
  if (pMiner->Location() != MinersShack)
  {
    cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Walkin' home";

    pMiner->ChangeLocation(MinersShack); 

    //let the wife know I'm home
    Dispatch->DispatchMessage(SEND_MSG_IMMEDIATELY, //time delay
                              pMiner->ID(),        //ID of sender
                              ent_Elsa,            //ID of recipient
                              Msg_HiHoneyImHome,   //the message
                              NO_ADDITIONAL_INFO);    
  }
}

void GoHomeAndSleepTilRested::Execute(Miner* pMiner)
{ 
  //if miner is not fatigued start to dig for nuggets again.
  if (!pMiner->Fatigued())
  {
     cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " 
          << "All mah fatigue has drained away. Time to find more gold!";

     pMiner->GetFSM()->ChangeState(EnterMineAndDigForNugget::Instance());
  }

  else 
  {
    //sleep
    pMiner->DecreaseFatigue();

    cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "ZZZZ... ";
  } 
}

void GoHomeAndSleepTilRested::Exit(Miner* pMiner)
{ 
}

bool GoHomeAndSleepTilRested::OnMessage(Miner* pMiner, const Telegram& msg)
{
   SetTextColor(BACKGROUND_RED|FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE);

   switch(msg.Msg)
   {
   case Msg_StewReady:

     cout << "\nMessage handled by " << GetNameOfEntity(pMiner->ID()) 
     << " at time: " << Clock->GetCurrentTime();

     SetTextColor(FOREGROUND_RED|FOREGROUND_INTENSITY);

     cout << "\n" << GetNameOfEntity(pMiner->ID()) 
          << ": Okay Hun, ahm a comin'!";

     pMiner->GetFSM()->ChangeState(EatStew::Instance());
      
     return true;

   }//end switch

   return false; //send message to global message handler
}



//------------------------------------------------------------------------QuenchThirst

QuenchThirst* QuenchThirst::Instance()
{
  static QuenchThirst instance;

  return &instance;
}

void QuenchThirst::Enter(Miner* pMiner)
{
  if (pMiner->Location() != saloon)
  {    
    pMiner->ChangeLocation(saloon);

    cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Boy, ah sure is thusty! Walking to the saloon";
  }

  //let the drunkard know Bob is in the saloon
  Dispatch->DispatchMessage(SEND_MSG_IMMEDIATELY, //time delay
	  pMiner->ID(),				  //ID of sender
	  ent_Drunkard,				  //ID of recipient
	  Msg_HiDrunkardImAtSaloon,   //the message
	  NO_ADDITIONAL_INFO);
}

void QuenchThirst::Execute(Miner* pMiner)
{
  pMiner->BuyAndDrinkAWhiskey();

  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "That's mighty fine sippin' liquer";

  pMiner->GetFSM()->ChangeState(EnterMineAndDigForNugget::Instance());  
  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Leaving the saloon";
}

void QuenchThirst::Exit(Miner* pMiner)
{ 
  
}

bool QuenchThirst::OnMessage(Miner* pMiner, const Telegram& msg)
{
	SetTextColor(BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	switch (msg.Msg)
	{
	case Msg_DrunkardDrunk:

		cout << "\nMessage handled by " << GetNameOfEntity(pMiner->ID())
			<< " at time: " << Clock->GetCurrentTime();

		SetTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY);

		cout << "\n" << GetNameOfEntity(pMiner->ID())
			<< ": Cmon drunkar, am waitin fer yah' !";

		pMiner->GetFSM()->ChangeState(Fighting::Instance());

		Dispatch->DispatchMessage(SEND_MSG_IMMEDIATELY, //time delay
			pMiner->ID(),        //ID of sender
			ent_Drunkard,          //ID of recipient
			Msg_LetsFight,			//the message
			NO_ADDITIONAL_INFO);

		return true;

	case Msg_LetsFight:

		cout << "\nMessage handled by " << GetNameOfEntity(pMiner->ID())
			<< " at time: " << Clock->GetCurrentTime();

		SetTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY);

		cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": "
			<< "*HIC!* Heya ! I saw ya lookin' at me the wrong way !";
		pMiner->GetFSM()->ChangeState(Fighting::Instance());

		return true;

	}//end switch

	return false; //send message to global message handler
}



//------------------------------------------------------------------------EatStew

EatStew* EatStew::Instance()
{
  static EatStew instance;

  return &instance;
}

void EatStew::Enter(Miner* pMiner)
{
  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Smells Reaaal goood Elsa!";
}

void EatStew::Execute(Miner* pMiner)
{
  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Tastes real good too!";

  pMiner->GetFSM()->RevertToPreviousState();
}

void EatStew::Exit(Miner* pMiner)
{ 
  cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Thankya li'lle lady. Ah better get back to whatever ah wuz doin'";
}

bool EatStew::OnMessage(Miner* pMiner, const Telegram& msg)
{
  //send msg to global message handler
  return false;
}



//------------------------------------------------------------------------methods for Brawling

Fighting* Fighting::Instance()
{
	static Fighting instance;

	return &instance;
}

void Fighting::Enter(Miner* pMiner)
{
	cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": "
		<< "You have no idea who're you're messing with !";
}

void Fighting::Execute(Miner* pMiner)
{
	Dispatch->DispatchMessage(SEND_MSG_IMMEDIATELY, //time delay
		pMiner->ID(),        //ID of sender
		ent_Drunkard,          //ID of recipient
		Msg_FightRound,		//the message
		(void*)pMiner->GetFatigueLevel());
}

void Fighting::Exit(Miner* pMiner)
{
}

bool Fighting::OnMessage(Miner* pMiner, const Telegram& msg)
{
	SetTextColor(BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

	switch (msg.Msg)
	{
	case Msg_FightRound:

		cout << "\nMessage handled by " << GetNameOfEntity(pMiner->ID())
			<< " at time: " << Clock->GetCurrentTime();

		SetTextColor(FOREGROUND_RED | FOREGROUND_INTENSITY);

		if ((int)msg.ExtraInfo >= pMiner->GetFatigueLevel())
		{
			// Miner won the fight
			cout << "\n" << GetNameOfEntity(pMiner->ID())
				<< ": Told'ya not to mess with me !";
			cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Let's drink again to celebrate this easy victory !";

			pMiner->GetFSM()->ChangeState(QuenchThirst::Instance());
		}
		else
		{
			// Miner lost the fight
			cout << "\n" << GetNameOfEntity(pMiner->ID())
				<< ": Can't believe I lost 'gainst someone of ya kind !";
			cout << "\n" << GetNameOfEntity(pMiner->ID()) << ": " << "Leaving the saloon";
			pMiner->GetFSM()->ChangeState(GoHomeAndSleepTilRested::Instance());
		}

		Dispatch->DispatchMessage(SEND_MSG_IMMEDIATELY, //time delay
			pMiner->ID(),        //ID of sender
			ent_Drunkard,          //ID of recipient
			Msg_FightRound,		//the message
			(void*)pMiner->GetFatigueLevel());
		
		return true;

	}//end switch

	return false; //send message to global message handler
}


