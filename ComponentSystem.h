#pragma once

#include <string>
#include <iostream>
#include <list>
#include <map>
#include <vector>
#include <memory>
#include <typeinfo>

#define CFID_UNKNOWN 0
#undef IN
#define IN const
#define OUT

typedef unsigned int	family_t;
typedef unsigned int	entity_t;
typedef unsigned int	cid_t;

struct Component {
	Component() : mUniqueId(0), mFamilyId( CFID_UNKNOWN ), mEntityId(0) {}
	cid_t	mUniqueId;
	entity_t mEntityId;
	family_t mFamilyId;
};

#if defined( WIN32 )
typedef std::tr1::shared_ptr<Component> ComponentPtr;
#elif defined(__APPLE__) && defined(__MACH__)
typedef std::shared_ptr<Component> ComponentPtr;
#endif

typedef std::vector< ComponentPtr >				component_vector;
typedef std::map< entity_t, component_vector >	component_map;

template<typename Type> inline Type smart_cast( ComponentPtr ptr ) { 
	return static_cast<Type>(ptr.get()); 
}

template<typename Type> inline Type safe_cast( ComponentPtr ptr ) { 
	if( ptr )
		return static_cast<Type>(ptr.get()); 
	
	return NULL;
}
inline bool type_of( ComponentPtr ptr, family_t familyId ) { return ptr->mFamilyId == familyId; }

typedef std::vector< entity_t >	entity_array;
typedef std::list< entity_t >	entity_list;
typedef std::list< entity_t >::iterator	entity_list_iterator;

class EntitySystem {
	entity_list		mErasedIds;
	entity_array	mEntities;
public:
	EntitySystem()
	{
		// 0 is undefined value, treated for handling errors
		mEntities.push_back(0);
	}
	~EntitySystem() {
		mEntities.clear();
		mErasedIds.clear();
	}

	entity_t	CreateNewEntity() {
		if( mErasedIds.empty() )
		{
			mEntities.push_back( mEntities.size() );
			return mEntities.size()-1;
		}
		else
		{
			// yes. get old erased id then replace it with a new component
			entity_t erasedId = mErasedIds.front();
			mErasedIds.pop_front();
			mEntities[ erasedId ] = erasedId;
			return erasedId;
		}
	}
	/// <summary>	Creates new entity under specific identifier. Gasps will be reserved and erased.
	/// 			In case entity ID is already reserved, function will fail. </summary>
	entity_t	CreateNewEntityUnderId( entity_t entityId ) {
		if( entityId > 0 )
		{
			if( Exist( entityId ) )
				return 0;

			// check under erased ID-s
			if( mErasedIds.empty() == false )
			{
				// look for entity ID under erased ID-s
				for( entity_list_iterator erasedId = mErasedIds.begin(); erasedId != mErasedIds.end(); erasedId++ ) {
					if( *erasedId == entityId ) {
						// we have found it
						mErasedIds.erase( erasedId );
						mEntities[ entityId ] = entityId;

						return entityId;
					}
				}
			}

			// we don't have any erased entities or its not found under erased ones
			// entity ID will be next in row?
			if( entityId == mEntities.size() )	{
				// yes. just create and return normally.
				mEntities.push_back( mEntities.size() );
				return mEntities.size()-1;
			}

			// on this point it can't be lesser than size (either its in erased ID's or its took )
			if( entityId < mEntities.size() )
				return 0;

			for( size_t i = mEntities.size(); i<entityId;  i++ ) {
				mEntities.push_back( i );
				mErasedIds.push_back( i );
			}

			mEntities.push_back( entityId );
			return entityId;
		}

		return 0;
	}
	entity_t	size()
	{
		return mEntities.size();
	}

	bool Delete( entity_t entityId )
	{
		if( Exist( entityId ) )
		{
			if( entityId != mEntities.back() )
				mErasedIds.push_back( entityId );
			else
				mEntities.pop_back();

			// check if last items are erased, if so, reduce array size
			while( mEntities.empty() == false ) {
				size_t lastId = mEntities.back();

				bool erased = false;
				// also check if there is such a UID in erased ID's list
				for( entity_list::iterator it = mErasedIds.begin(); it!=mErasedIds.end(); ++it ) {
					if( *it == lastId )
					{
						mEntities.pop_back();
						mErasedIds.erase( it );
						erased = true;
						break;
					}
				}
				if( !erased )
					return true;
			}
			return true;
		}

		return false;
	}

	bool Exist( entity_t parentId ) {
		// if size is ok
		if( parentId < mEntities.size() )
		{
			// and if id is not under erased ID-s
			for( entity_list_iterator i = mErasedIds.begin(); i != mErasedIds.end(); i++ )
				if( *i == parentId )
					return false;

			// then exists
			return true;
		}

		return false;
	}
	void Clear(){
		mErasedIds.clear();
		mEntities.clear();
		mEntities.push_back(0);
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////
/// <summary>	Component system. Class for handling component, and their memory management.  </summary>
////////////////////////////////////////////////////////////////////////////////////////////////////

class ComponentSystem {
protected:
	EntitySystem		entitySystem;
private:
	/// <summary>	component container. </summary>
	component_vector mComponentArray;
	/// <summary>	List of erased unique identifiers. </summary>
	entity_list	mErasedIds;

	// used for faster fetching data based on entity ID and on family ID
	std::vector< component_vector > mEntityComponentArray;
	//component_map mEntityComponentMap;
	component_map mFamilyComponentMap;
public:

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Default constructor. </summary>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	ComponentSystem() {
		mComponentArray.push_back( ComponentPtr() );

		/// <summary>	The dummy component. Used for return values. Similar to smart NULL. </summary>
		mComponentArray[0].reset();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Destructor. </summary>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	~ComponentSystem() {
		mComponentArray.clear();
		mErasedIds.clear();
		//mEntityComponentMap.clear();
		mEntityComponentArray.clear();
		mFamilyComponentMap.clear();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Creates new entity under specific identifier. Currently used for loading. </summary>
	/// <param name="entityId">	Identifier for the entity. </param>
	/// <returns>	The new new entity under identifier. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	entity_t CreateNewEntityUnderId( IN entity_t entityId )
	{
		return entitySystem.CreateNewEntityUnderId( entityId );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	
	/// 		Attaches new given component to component system. If there was a previously deleted
	/// 		object, deleted objects space will be reused, and deleted object's unique Id attached to 
	/// 		newly created component.
	/// </summary>
	/// <param name="newComponent">	[in] If non-null, the new component. </param>
	/// <returns>	Attached component. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename Type>	ComponentPtr CreateComponent( IN entity_t entityId ) {

		// do we have erased components?
		if( mErasedIds.empty() )
		{
			Type* newComponent = new Type;
			
			// no. put new component into the system
			newComponent->mEntityId	= entityId;
			mComponentArray.push_back( ComponentPtr( newComponent ) );

			newComponent->mUniqueId = (cid_t)mComponentArray.size()-1;

			// map to entity components
			if( entityId >= mEntityComponentArray.size() )
				mEntityComponentArray.resize( entityId + 1 );

			mEntityComponentArray[ entityId ].push_back( mComponentArray.back() );
			mFamilyComponentMap[ newComponent->mFamilyId ].push_back( mComponentArray.back() );
			return mComponentArray.back();
		}
		else
		{
			// yes. get old erased id then replace it with a new component
			entity_t erasedId = mErasedIds.front();
			mErasedIds.pop_front();
			return Replace<Type>( erasedId, entityId );
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Attach the component. </summary>
	/// <param name="component">	The component. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	inline bool AttachComponent( IN ComponentPtr& component )
	{
		mComponentArray.push_back( component );

		if( component->mEntityId >= mEntityComponentArray.size() )
			mEntityComponentArray.resize( component->mEntityId + 1 );

		mEntityComponentArray[ component->mEntityId ].push_back( component );
		mFamilyComponentMap[ component->mFamilyId ].push_back( component );
		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Attach array of components. </summary>
	/// <param name="componentArray">	Array of components. </param>
	/// <returns>	true if it succeeds, false if it fails. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	bool AttachArray( IN component_vector& componentArray )
	{
		for( size_t i = 0;i <componentArray.size(); i++ ) {

			AttachComponent( componentArray[i]);
		}

		return true;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	
	/// 		Attaches the given new component to component system. If there was a previously deleted
	/// 		object, deleted objects space will be reused, and deleted object's unique Id attached to 
	/// 		newly created component. Components can be multi attached in one row as:
	/// 			componentSystem
	///					->	MultiAttach<Vertex>				( new Vertex )
	///					->	MultiAttach<UV>					( new UV );
	/// </summary>
	/// <param name="newComponent">	[in] If non-null, the new component. </param>
	/// <returns>	This instance. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	/*template<typename Type>	ComponentSystem* MultiAttach( IN entity_t entityId ) {

	Attach( entityId );
	return this;
	}*/

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>
	/// 	Replaces the component based on component's unique identifier from component system.
	/// 	Component will be replaced only if ownership of smart pointer is held by one object.
	/// 	Multiple ownerships need to be handled, and components from component system's outer
	/// 	scope need to be erased first for this method to succeed.
	/// </summary>
	/// <typeparam name="typename Type">	Type of the typename type. </typeparam>
	/// <param name="uniqueId">	Unique identifier. </param>
	/// <param name="entityId">	Identifier for the entity. </param>
	///
	/// <returns>
	/// 	Newly attached component. In case of error empty smart pointer is returned.
	/// </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename Type>	ComponentPtr Replace( IN cid_t uniqueId, IN entity_t entityId ) {
		// only allow delete of the pointer in case there is no instance of object	
		if( RefCount( uniqueId ) == 0 )
		{
			if( uniqueId >= mComponentArray.size() )
			{
				mComponentArray.resize(uniqueId+1);
			}

			Type* newComponent = new Type;
			newComponent->mUniqueId = uniqueId;
			newComponent->mEntityId = entityId;


			mComponentArray[ uniqueId ] = ComponentPtr( newComponent );

			if( entityId >= mEntityComponentArray.size() )
				mEntityComponentArray.resize( entityId + 1 );

			mEntityComponentArray[ entityId ].push_back( mComponentArray[ uniqueId ] );
			mFamilyComponentMap[ newComponent->mFamilyId ].push_back( mComponentArray[ uniqueId ] );

			return mComponentArray[ uniqueId ];
		}

		return mComponentArray[0];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	
	/// 	Releases the component pointer based on component's unique identifier from component system. 
	/// 	Component will be released if ownership of smart pointer is held by only one object.
	/// 	Multiple ownerships need to be handled, and components from component system's outer scope need to
	/// 	be released first for this method to succeed.
	/// </summary>
	/// <param name="uniqueId">	Unique identifier. </param>
	///
	/// <returns>	true if it succeeds, false if it fails. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	bool Release( IN cid_t uniqueId )
	{
		// only allow delete of the pointer in case there is 
		// only one instance of object in each of:
		if( RefCount( uniqueId ) == 0 )
		{
			entity_t entityId = mComponentArray[uniqueId]->mEntityId;
			for( entity_t i = 0; i<mEntityComponentArray[ entityId ].size(); i++ )
			{
				if( mEntityComponentArray[ entityId ][ i ]->mUniqueId == uniqueId )
				{
					mEntityComponentArray[ entityId ].erase( mEntityComponentArray[ entityId ].begin() + i );
					break;
				}
			}

			family_t familyId = mComponentArray[uniqueId]->mFamilyId;
			for( entity_t i = 0; i<mFamilyComponentMap[ familyId ].size(); i++ )
			{
				if( mFamilyComponentMap[ familyId ][ i ]->mUniqueId == uniqueId )
				{
					mFamilyComponentMap[ familyId ].erase( mFamilyComponentMap[ familyId ].begin() + i );
					break;
				}
			}
			// clear but don't erase
			mComponentArray[ uniqueId ].reset();
			mErasedIds.push_back( uniqueId );
			return true;
		}

#ifdef _DEBUG
		/*std::cout 
		<< "Can't release id " << std::to_string( (_Longlong) uniqueId) << "."
		<< "Id " << std::to_string( (_Longlong) uniqueId ) << " have refcount of " << 
		std::to_string( (_Longlong) RefCount(uniqueId) ) << std::endl;*/
#endif
		return false;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Reference count. </summary>
	/// <param name="uniqueId">	Unique identifier. </param>
	/// <returns>	Returns number of references of given object. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	size_t RefCount( IN cid_t uniqueId )
	{
		// check for out of bounds
		if( uniqueId < mComponentArray.size() )
		{
			if( mComponentArray[ uniqueId ] )
				// -3 because of 3 map arrays of storing data for access
				return mComponentArray[ uniqueId ].use_count()-3;
		}
		return 0;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Gets a component based on its uniqueID. </summary>
	/// <param name="unqiueId">	Unqiue identifier for the component. </param>
	/// <returns>	The component. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	const ComponentPtr& GetComponent( IN cid_t unqiueId ) 
	{ 
		if( unqiueId < mComponentArray.size() )
			return mComponentArray[unqiueId]; 

		return mComponentArray[0];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Gets a component list based on entityId. </summary>
	/// <param name="entityId">		 	Unqiue identifier for the entity. </param>
	/// <param name="componentsList">	[out] List of components. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void GetComponentsByEntity( IN entity_t entityId, OUT component_vector& componentsList )
	{
		componentsList = mEntityComponentArray[ entityId ];
	}

	void AppendComponentsByEntity( IN entity_t entityId, OUT component_vector& componentsList )
	{
		if( entityId >= mEntityComponentArray.size() )
			mEntityComponentArray.resize( entityId + 1 );

		componentsList.insert( componentsList.end(), mEntityComponentArray[ entityId ].begin(), mEntityComponentArray[ entityId ].end() );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Gets a component list based on components family id. </summary>
	/// <param name="familyId">		 	Unqiue identifier for the family. </param>
	/// <param name="componentsList">	[out] List of components. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void GetComponentsByFamily( IN family_t familyId, OUT component_vector& componentsList )
	{
		componentsList = mFamilyComponentMap[ familyId ];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Appends component list based on components family and entity id. </summary>
	/// <param name="entityId">		 	Unqiue identifier for the entity. </param>
	/// <param name="familyId">		 	Unqiue identifier for the family. </param>
	/// <param name="componentsList">	[out] List of components. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void GetComponentsByEntityAndFamily( IN entity_t entityId, IN family_t familyId, OUT component_vector& componentsList )
	{
		for( entity_t i =0; i< mEntityComponentArray[entityId].size(); i++ )
			if( mEntityComponentArray[ entityId ][i]->mFamilyId == familyId )
				componentsList.push_back( mEntityComponentArray[ entityId ][i] );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>
	/// 	Appends component list based on components family and entity id from external container.
	/// </summary>
	///
	/// <param name="container">	 	[in] The container. </param>
	/// <param name="entityId">		 	Unqiue identifier for the entity. </param>
	/// <param name="familyId">		 	Unqiue identifier for the family. </param>
	/// <param name="componentsList">	[out] List of components. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void GetComponentsByEntityAndFamily( std::vector<component_vector>& container, IN entity_t entityId, IN family_t familyId, OUT component_vector& componentsList )
	{

		for( entity_t i =0; i<container[entityId].size(); i++ )
			if( container[ entityId ][i]->mFamilyId == familyId )
				componentsList.push_back( container[ entityId ][i] );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Gets components by family and entity. </summary>
	///
	/// <param name="entityId">		 	Identifier for the entity. </param>
	/// <param name="familyId">		 	Identifier for the family. </param>
	/// <param name="componentsList">	[in,out] List of components. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void GetComponentsByFamilyAndEntity( IN entity_t entityId, IN family_t familyId, OUT component_vector& componentsList )
	{
		for( family_t i =0; i< mFamilyComponentMap[familyId].size(); i++ )
			if( mFamilyComponentMap[ familyId ][i]->mEntityId == entityId )
				componentsList.push_back( mFamilyComponentMap[ familyId ][i] );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Searches for the first component by entity and family. </summary>
	///
	/// <param name="entityId">	Identifier for the entity. </param>
	/// <param name="familyId">	Identifier for the family. </param>
	///
	/// <returns>	The found component by entity and family. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	ComponentPtr FindFirstComponentByEntityAndFamily( IN entity_t entityId, IN family_t familyId )
	{
		if( entityId < mEntityComponentArray.size() )
		{
			for( entity_t i =0; i< mEntityComponentArray[entityId].size(); i++ )
			{
				if( mEntityComponentArray[ entityId ][i]->mFamilyId == familyId )
				{
					return mEntityComponentArray[ entityId ][i];
				}
			}
		}

		return mComponentArray[0];
	}

	ComponentPtr FindFirstComponentByFamily( IN family_t familyId )
	{
		if( mFamilyComponentMap[familyId].size() ) {
			return mFamilyComponentMap[ familyId ][0];
		}

		return mComponentArray[0];
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Clears this object to its blank/initial state. </summary>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void Clear() {

		mComponentArray.clear();
		mErasedIds.clear();
		mEntityComponentArray.clear();
		mFamilyComponentMap.clear();

		/// <summary>	The dummy component. Used for return values. </summary>
		mComponentArray.push_back( ComponentPtr() );
		mComponentArray[0].reset();
		entitySystem.Clear();
	}

	void Resize( size_t size ) {
		mComponentArray.resize( size );
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Gets a first component by its type. </summary>
	///
	/// <typeparam name="typename Type">	Type of the typename type. </typeparam>
	/// <param name="entityId">	Identifier for the entity. </param>
	/// <param name="familyId">	Identifier for the family. </param>
	///
	/// <returns>	. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename Type> inline Type Get( IN entity_t entityId, IN family_t familyId ) 
	{ 
		ComponentPtr ptr = FindFirstComponentByEntityAndFamily( entityId, familyId );

		if( ptr )
			return smart_cast<Type>(ptr); 

		return NULL;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Rebuild erased ids. </summary>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void RebuildErasedIDs() {
		mErasedIds.clear();
		for( entity_t i = 1; i<Size(); i++ )
		{
			if( !mComponentArray[i] )	// or use_count() == 0 ?
				mErasedIds.push_back(i);
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Gets the size of components. </summary>
	/// <returns>	. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	entity_t Size() {
		return (entity_t)mComponentArray.size();
	}

	entity_t EntitySize() {
		return entitySystem.size();
	}

	entity_t ErasedIDSize() {
		return mErasedIds.size();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Count components by entity and family. </summary>
	///
	/// <param name="entityId">	Identifier for the entity. </param>
	/// <param name="familyId">	Identifier for the family. </param>
	///
	/// <returns>	The total number of components by entity and family. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	entity_t CountComponentsByEntityAndFamily( IN entity_t entityId, IN family_t familyId )
	{
		entity_t size = 0;
		for( entity_t i =0; i< mEntityComponentArray[entityId].size(); i++ )
			if( mEntityComponentArray[ entityId ][i]->mFamilyId == familyId )
				size++;

		return size;
	}

	bool DeleteComponent( IN cid_t componentId ) {

		if( componentId < mComponentArray.size() )
		{
			family_t componentFamily	= mComponentArray[ componentId ]->mFamilyId;
			entity_t entityType			= mComponentArray[ componentId ]->mEntityId;
			entity_t uniqueId			= mComponentArray[ componentId ]->mUniqueId;

			bool erased = false;
			for( size_t i = 0; i<mFamilyComponentMap[ componentFamily ].size(); i++ ){
				if( mFamilyComponentMap[ componentFamily ][i]->mUniqueId == uniqueId ) {

					mFamilyComponentMap[ componentFamily ].erase(mFamilyComponentMap[ componentFamily ].begin() + i);
					erased = true;
					break;
				}
			}

			if( !erased )
				return false;

			erased = false;

			for( size_t i = 0; i<mEntityComponentArray[ entityType ].size(); i++ ){
				if( mEntityComponentArray[ entityType ][i]->mUniqueId == uniqueId ) {
					mEntityComponentArray[ entityType ].erase(mEntityComponentArray[ entityType ].begin() + i);
					erased = true;
					break;
				}
			}

			if( !erased )
				return false;
			mComponentArray[ componentId ].reset();
			// if last component doesn't need to be put under erased ID's
			if( componentId == mComponentArray.size()-1 )
				mComponentArray.pop_back();
			else
				mErasedIds.push_back( componentId );
		}
		return true;
	}
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Deletes the given entity based on entityId. </summary>
	///
	/// <param name="entityId">	The entity identifier to delete. </param>
	///
	/// <returns>	true if it succeeds, false if it fails. </returns>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	bool DeleteEntity( IN entity_t entityId ) {

		if( entitySystem.Delete( entityId ) ) 
		{
			component_vector components;
			GetComponentsByEntity( entityId, components );

			// erase from family map
			for( entity_t i = 0; i< components.size(); i++ ) {

				mErasedIds.push_back( components[i]->mUniqueId );

				family_t componentFamily	= components[i]->mFamilyId;
				entity_t uniqueId			= components[i]->mUniqueId;

				for( size_t i = 0; i<mFamilyComponentMap[ componentFamily ].size(); i++ ){
					if( mFamilyComponentMap[ componentFamily ][i]->mUniqueId == uniqueId ) {
						mFamilyComponentMap[ componentFamily ].erase(mFamilyComponentMap[ componentFamily ].begin()+i);
					}
				}

				if( mFamilyComponentMap[ componentFamily ].empty() )
					mFamilyComponentMap.erase( componentFamily );
			}

			mEntityComponentArray[entityId].clear();

			for( entity_t i = 0; i< components.size(); i++ ) {
				mComponentArray[ components[i]->mUniqueId ].reset();
			}

			// check if last items are erased, if so, reduce array size
			while( mComponentArray.empty() || (mComponentArray.back() == false) ) {
				size_t lastId = mComponentArray.size()-1;
				mComponentArray.pop_back();

				// also check if there is such a UID in erased ID's list
				for( entity_list::iterator it = mErasedIds.begin(); it!=mErasedIds.end(); ++it ) {
					if( *it == lastId )
					{
						mErasedIds.erase( it );
						break;
					}
				}
			}

			return true;
		}

		return false;
	}
protected:
	template<typename Type>	ComponentPtr DuplicateComponent( IN entity_t newEntityId, IN ComponentPtr& sourceComponent ) {

		ComponentPtr ptrCreatedComponent = CreateComponent<Type>( newEntityId );

		Type* ptrOriginalType	= smart_cast<Type*>( sourceComponent );
		Type* ptrCopiedType		= smart_cast<Type*>( ptrCreatedComponent );

		entity_t uniqueId = ptrCreatedComponent->mUniqueId;
		entity_t entityId = ptrCreatedComponent->mEntityId;

		*ptrCopiedType = *ptrOriginalType;

		// HACK: assign operator will overwrite ALL hierarchy data
		// we just put back two item of component which must be unique for 
		// duplicated object
		ptrCreatedComponent->mUniqueId = uniqueId;
		ptrCreatedComponent->mEntityId = entityId;

		return ptrCreatedComponent;
	}
public:
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Dumps components attached to this object. </summary>
	////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
	void Dump() {
		std::cout << "--- List of components ---" << std::endl;
		for( entity_t i = 0; i< mComponentArray.size(); i++ )
		{
			if( mComponentArray[i] )
			{
				DumpComponent( mComponentArray[i]->mUniqueId );
			}
			else
			{
				std::cout 
					<< std::to_string( (_Longlong)  i ) 
					<< " empty" << std::endl;
			}
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Dumps an entity based on entity id. </summary>
	/// <param name="entityId">	Identifier for the entity. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void DumpEntity( entity_t entityId )
	{
		for( entity_t i = 0; i< mEntityComponentArray[ entityId ].size(); i++ )
		{
			DumpComponent( mEntityComponentArray[ entityId ][i]->mUniqueId );
		}
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary>	Dumps a component based on components unique id. </summary>
	/// <param name="uniqueId">	Unique identifier. </param>
	////////////////////////////////////////////////////////////////////////////////////////////////////

	void DumpComponent( entity_t uniqueId ) {

		if( mComponentArray.size() > uniqueId && mComponentArray[ uniqueId ] )
		{
			std::cout 
				<< " UID(" << std::to_string( (_Longlong)  mComponentArray[ uniqueId ]->mUniqueId )  << ") " 
				<< " EID(" << std::to_string( (_Longlong)  mComponentArray[ uniqueId ]->mEntityId ) 	<< ") " 
				<< " FID(" << std::to_string( (_Longlong)  mComponentArray[ uniqueId ]->mFamilyId ) 	<< ") " 
				<< " RefCount(" << std::to_string( (_Longlong)  RefCount( uniqueId ) )	<< ") " 
				//<< typeid(mComponentArray[ uniqueId ]).name()
				//<< mComponentArray[ uniqueId ]->whois() 
				<< std::endl;
		}
		else
		{
			std::cout 
				<< std::to_string( (_Longlong)  uniqueId ) 
				<< " empty" << std::endl;
		}
	}
#else
	void Dump() {}
	void DumpEntity( entity_t ) {}
	void DumpComponent( entity_t )	{}
#endif
};
