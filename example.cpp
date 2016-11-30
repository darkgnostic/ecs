// ECS.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ComponentSystem.h"
#include <iostream>

#define CFID_HEALTH			1
#define CFID_ARMOR			2
#define CFID_ATTACK			3
#define CFID_NAME			4

// component holding entity's name
struct Name : public Component {
	Name() : name("") { mFamilyId = CFID_NAME; }
	std::string name;
};

// component holding entity's health
struct Health : public Component {
	Health() : health(10) { mFamilyId = CFID_HEALTH; }
	int health;
};

// component holding entity's armor
struct Armor : public Component {
	Armor() : armor(3) { mFamilyId = CFID_ARMOR; }
	int armor;
};

// component holding entity's attack power
struct Attack : public Component {
	Attack() : strength(2) { mFamilyId = CFID_ATTACK; }
	int strength;
};

// System responsible of creating tanks
class TankFactory : public ComponentSystem {
public:
	entity_t Create( const std::string& name ) {

		// create new entity
		entity_t eid = entitySystem.CreateNewEntity();

		// add attack, health, name and armor components to entity
		CreateComponent<Attack>(eid);

		ComponentPtr com_armor = CreateComponent<Armor>(eid);
		ComponentPtr com_health = CreateComponent<Health>(eid);

		// randomize a bit health and armor
		smart_cast<Health*>(com_health)->health = 5+rand()%5;
		smart_cast<Armor*>(com_armor)->armor += (rand()%2)-1;

		smart_cast<Name*>( CreateComponent<Name>(eid) )->name = name;
		
		return eid;
	}
};

// system responsible of battling  tanks
class TankBattleSystem : public ComponentSystem {
public:
	bool MakeAttack( entity_t attacker, entity_t defender ) {

		// get attackers attack ppower and defender's armor
		Attack* attack = Get<Attack*>( attacker, CFID_ATTACK );
		Armor* armor = Get<Armor*>( defender, CFID_ARMOR );

		int attack_power = rand()%6;

		// make some kind of attack 
		if( attack_power < attack->strength ) {

			// attack succeeded
			// reduce defender's health
			Health* defender_health = Get<Health*>( defender, CFID_HEALTH );
			defender_health->health--;

			// print some stat messages
			std::cout << Get<Name*>( attacker, CFID_NAME )->name << " reduces " <<
				Get<Name*>( defender, CFID_NAME )->name << "'s health with 1 damage to " <<
				defender_health->health << " health." <<
				std::endl;

			// and return true if defender died.
			if( defender_health->health <= 0 )
				return true;
		} else {
			std::cout << Get<Name*>( attacker, CFID_NAME )->name << " misses " <<
				Get<Name*>( defender, CFID_NAME )->name <<std::endl;
		}

		return false;
	}
};

int _tmain(int argc, _TCHAR* argv[])
{
	TankFactory tankFactory;

	// create two tanks
	entity_t tank1 = tankFactory.Create("Sherman");
	entity_t tank2 = tankFactory.Create("Panzer");

	// fetch all components of two tanks based on their enity id
	component_vector vec_tank1_components, vec_tank2_components;
	tankFactory.GetComponentsByEntity(tank1, vec_tank1_components );
	tankFactory.GetComponentsByEntity(tank2, vec_tank2_components );

	// instantiate battle system
	TankBattleSystem battleSystem;

	// and add to it two tanks previously defined
	battleSystem.
		AttachArray(vec_tank1_components)->
		AttachArray(vec_tank2_components);

	// loop a battle between two tanks until one is dead.
	bool battle_ongoing = true;
	while( battle_ongoing ) {

		// if make attack returns true, then the defender is dead
		if( battleSystem.MakeAttack( tank1, tank2 ) == false ) {
			if( battleSystem.MakeAttack( tank2, tank1 ) ) {
				std::cout << tankFactory.Get<Name*>(tank2, CFID_NAME)->name << " wins." << std::endl; 
				battle_ongoing = false;
			}
		} else {
			std::cout << tankFactory.Get<Name*>(tank1, CFID_NAME)->name << " wins." << std::endl; 
			battle_ongoing = false;
		}
	}

	return 0;
}

