#pragma once

#include "rect_traits.h"
#include "vector_traits.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace primary
{
	template<typename TreeType, typename ValueT, typename PointerT, typename ReferenceT>
	class tree_iterator_base;
	template<typename TreeType> class tree_iterator;
	template<typename TreeType> class const_tree_iterator;

	template<
		std::size_t allowed_objects_per_node,
		typename RectTraits,
		typename VecTraits,
		typename Object>
		class qtree {
		public:
			using rect_traits = RectTraits;
			using vec_traits = VecTraits;

			using rect_type = typename rect_traits::rect_type;
			using vec_type = typename vec_traits::vector_type;

			using value_type = Object;
			using pointer = Object*;
			using reference = Object&;
			using const_pointer = Object const*;
			using const_reference = Object const&;

			using iterator = typename std::vector<value_type>::iterator;
			using const_iterator = typename std::vector<value_type>::const_iterator;
			static constexpr std::size_t max_objects = allowed_objects_per_node;

		public:
			class data {
			public:
				data( rect_type const& bounds, pointer pObj )noexcept
					:
					m_bounds( bounds ),
					m_pObject( pObj )
				{}

				rect_type const& bounds()const noexcept {
					return m_bounds;
				}

				value_type const& object()const noexcept {
					return *m_pObject;
				}

				value_type& object()noexcept {
					return *m_pObject;
				}

			private:
				rect_type m_bounds = {};
				pointer m_pObject = nullptr;
			};

			class node {
			public:
				node( rect_type const& bounds_ )
					:
					m_bounds( bounds_ )
				{
					m_data.reserve( max_objects );
				}

				std::vector<data>& elements()noexcept {
					return m_data;
				}

				std::vector<data> const& elements()const noexcept {
					return m_data;
				}

				rect_type const& bounds()const noexcept {
					return m_bounds;
				}
			private:
				using find_result = std::pair<node*, typename std::vector<data>::iterator>;

				void query( rect_type const& bounds, std::vector<pointer>& contained ) {
					if( rect_traits::intersects( m_bounds, bounds ) ) {
						for( auto& child : m_pChildren ) {
							if( child )
								child->query( bounds, contained );
						}

						contained.reserve( m_data.size() + contained.size() );

						for( auto& element : m_data ) {
							if( rect_traits::intersects( element.bounds(), bounds ) )
								contained.push_back( &element.object() );
						}
					}
				}

				rect_type get_quadrant( int index )const noexcept {
					const auto left = rect_traits::left( m_bounds );
					const auto top = rect_traits::top( m_bounds );
					const auto right = rect_traits::right( m_bounds );
					const auto bottom = rect_traits::bottom( m_bounds );
					const auto center =
						rect_traits::template center<vec_traits>( m_bounds );

					switch( index ) {
						// left top
						case 0: return rect_traits::construct( left, top, center.x, center.y );
							// right top
						case 1: return rect_traits::construct( center.x, top, right, center.y );
							// left bottom 
						case 2: return rect_traits::construct( left, center.y, center.x, bottom );
							// right bottom
						case 3: return rect_traits::construct( center.x, center.y, right, bottom );
					}

					assert( index >= 0 && index < 4 && "Index out of range" );
					return rect_type{};
				}

				bool add_to_child( data const& data_ ) {
					if( m_data.size() >= max_objects ) return false;

					for( int index = 0; auto & child : m_pChildren ) {
						auto quadrant = get_quadrant( index++ );

						if( !rect_traits::contains( quadrant, data_.bounds() ) ) continue;
						if( !child ) child = std::make_unique<node>( quadrant );
						child->add_object( data_ );
						return true;
					}

					return false;
				}

				void add_object( data const& data_ ) {
					if( !add_to_child( data_ ) ) {
						m_data.push_back( data_ );
					}
				}

				find_result find_node( const_reference object, rect_type const& bounds ) {
					auto is_same = [&]( data const& data_ ) { return &data_.object() == &object; };
					auto it = std::find_if( m_data.begin(), m_data.end(), is_same );

					if( it != m_data.end() ) return { this, it };

					for( int index = 0; auto & child : m_pChildren ) {
						if( !child ) continue;
						if( !rect_traits::intersects( child->bounds(), bounds ) )continue;

						auto result = child->find_node( object, bounds );
						if( result.first != nullptr ) return result;
					}

					return { nullptr, m_data.end() };
				}

			private:
				friend class qtree;
				std::array<std::unique_ptr<node>, 4> m_pChildren;
				std::vector<data> m_data;
				rect_type m_bounds;
			};

			qtree( rect_type const& bounds_, std::function<rect_type( value_type const& )> get_rect_fn )
				:
				root( bounds_ ),
				get_rect( std::move( get_rect_fn ) )
			{}

			void push( value_type const& object ) {
				objects.push_back( object );
			}
			void push( value_type&& object ) {
				objects.push_back( std::move( object ) );
			}

			template<typename...Args>
			value_type& emplace( Args&&... args ) {
				return objects.emplace_back( std::forward<Args>( args )... );
			}

			void commit() {
				root = node( root.m_bounds );
				for( auto& object : objects ) {
					root.add_object( { get_rect( object ), &object } );
				}
			}

			void reserve( std::size_t count ) {
				if( count > objects.max_size() )
					throw std::invalid_argument( "max_size exceeded" );
				if( count > objects.size() )
					objects.reserve( count );
			}

			void clear()noexcept {
				root = node( root.m_bounds );
				objects.clear();
			}

			iterator begin()noexcept {
				return objects.begin();
			}
			iterator end()noexcept {
				return objects.end();
			}

			const_iterator begin()const noexcept {
				return objects.cbegin();
			}
			const_iterator end()const noexcept {
				return objects.cend();
			}

			std::vector<pointer> query( rect_type const& bounds ) {
				std::vector<pointer> objects;
				root.query( bounds, objects );
				return objects;
			}

			void remove_object( pointer pobject ) {
				if( objects.size() == 0 )return;

				auto is_same = [&]( value_type const& pobj ) { return &pobj == pobject; };
				auto obj_it = std::find_if( objects.begin(), objects.end(), is_same );
				if( obj_it != objects.end() )
				{
					// Remove objects from nodes since the 
					// nodes' object pointers will be invalid after removal
					for( auto it = obj_it; it != objects.end(); ++it ) {
						auto& removed = *it;
						auto [n, rem_it] = root.find_object( removed, get_rect( removed ) );
						if( n != nullptr )
							swap_and_pop( n->m_data, rem_it );
					}

					swap_and_pop( objects, obj_it );

					for( ; obj_it != objects.end(); ++obj_it ) {
						root.add_object( { get_rect( *obj_it ), *obj_it } );
					}
				}
			}

			auto find( const_reference object ) {
				return root.find_node( object, get_rect( object ) );
			}
		private:
			std::vector<value_type> objects;
			node root;
			std::function<rect_type( const value_type& )> get_rect;
	};

	template<
		typename TreeType,
		typename ValueT,
		typename PointerT,
		typename ReferenceT
	> class tree_iterator_base {
	public:
		using value_type = ValueT;
		using pointer = PointerT;
		using reference = ReferenceT;
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using tree_ptr = std::conditional_t<
			std::is_const_v<std::remove_pointer_t<pointer>>,
			const TreeType*,
			TreeType*>;
	public:
		tree_iterator_base() = default;
		tree_iterator_base( tree_ptr ptree_, pointer parent_, pointer current_, std::size_t cindex )
			:
			ptree( ptree_ ),
			current( current_ ),
			parent( parent_ ),
			child_index( cindex )
		{}


	protected:
		void update() {
			if( auto result = find_next_parent( current ); result != nullptr ) {
				parent = result;
				current = parent->m_pChildren[ child_index ].get();
			}
			else {
				parent = nullptr;
				current = &ptree->root;
				child_index = std::size_t{};
			}
		}
		pointer find_next_parent( pointer current_ )const {
			if( current_->m_pParent == nullptr )return nullptr;

			auto is_same_node = [&]( auto const& child ) {return child.get() == current_; };

			auto node_it = std::find_if(
				current_->m_pParent->m_pChildren.begin(),
				current_->m_pParent->m_pChildren.end(),
				is_same_node
			);

			auto pindex = std::distance( current_->m_pParent->m_pChildren.begin(), node_it );

			if( pindex < 3 )
				return current_->m_pParent;

			return find_next_parent( current_->m_pParent );
		}
		pointer find_next_child( pointer parent_ )const {
			for( auto& child : parent_->m_pChildren ) {
				if( child ) {
					return child.get();
				}
			}
			return pointer{};
		}
	protected:
		tree_ptr ptree = nullptr;
		pointer current = nullptr;
		pointer parent = nullptr;
		std::size_t child_index = {};
	};

	template<typename TreeType>
	class tree_iterator : tree_iterator_base<
		TreeType,
		typename TreeType::node,
		typename TreeType::node*,
		typename TreeType::node&
	> {
	public:
		using value_type = typename TreeType::node;
		using pointer = typename TreeType::node*;
		using reference = typename TreeType::node&;
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using my_base = tree_iterator_base<TreeType, value_type, pointer, reference>;
	public:
		using my_base::my_base;

		tree_iterator& operator++() {
			if( child_index + std::size_t{ 1 } < std::size_t{ 4 } ) {
				current = parent->m_pChildren[ ++child_index ].get();
			}
			else {
				auto pParent = find_next_parent( current );
				child_index = 0;

				if( pParent == nullptr ) {
					// parent is root, find first child
					for( auto& child : current->m_pChildren ) {
						if( !child )continue;
						parent = child.get();
						break;
					}
				}
				else {
					parent = pParent;
				}

				current = find_next_child( parent );
			}

			return *this;
		}
		tree_iterator operator++( int ) {
			auto copy = *this;
			++( *this );
			return copy;
		}
		tree_iterator& operator--() {
			if( child_index > 0 ) {
				auto find_prev_child = []( pointer parent_, std::size_t cindex ) {
					if( parent_ == nullptr ) {
						return std::size_t{};
					}

					for( auto i = std::size_t{}; i < cindex; ++i ) {
						if( parent_->m_pChildren[ i ] ) {
							return cindex - i;
						}
					}

					return std::size_t{};
				};

				auto index = find_prev_child( parent, child_index );

				// no siblings, move up to parent->parent->m_pChildren[3]
				if( index == std::size_t{} ) {
					auto temp_parent = parent->m_pParent;

					index = find_prev_child( parent, std::size_t{ 4 } );
				}
				if( parent->m_pChildren[ index ] ) {
					current = parent->m_pChildren[ index ].get();
				}
			}
			else {
				child_index = 3;
			}

			return *this;
		}
		tree_iterator operator--( int ) {
			auto copy = *this;
			--( *this );
			return copy;
		}

		reference operator*() {
			return *current;
		}
		pointer operator->() {
			return current;
		}
		/*friend bool operator==( tree_iterator const& lhs, tree_iterator const& rhs ) {
			return
				lhs.current == rhs.current &&
				lhs.parent == rhs.parent;
		}
		friend bool operator!=( tree_iterator const& lhs, tree_iterator const& rhs ) {
			return !( lhs == rhs );
		}*/

	};

	template<typename TreeType>
	class const_tree_iterator : tree_iterator_base<
		TreeType,
		typename TreeType::node,
		typename TreeType::node const*,
		typename TreeType::node const&
	> {
	public:
		using value_type = typename TreeType::node;
		using pointer = typename TreeType::node const*;
		using reference = typename TreeType::node const&;
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using my_base = tree_iterator_base<TreeType, value_type, pointer, reference>;

	public:
		using my_base::my_base;

		const_tree_iterator& operator++() {
			child_index = ( child_index + 1 ) % 4;
			update();
			return *this;
		}
		const_tree_iterator operator++( int ) {
			auto copy = *this;
			++( *this );
			return copy;
		}
		const_tree_iterator& operator--() {
			// Lol, subtract and wrap by adding and mod'ing
			child_index = ( child_index + 3 ) % 4;
			update();
			return *this;
		}
		const_tree_iterator operator--( int ) {
			auto copy = *this;
			--( *this );
			return copy;
		}

		reference operator*()const {
			return *current;
		}
		pointer operator->()const {
			return current;
		}

		/*friend bool operator==( const_tree_iterator const& lhs, const_tree_iterator const& rhs ) {
			return
				lhs.current == rhs.current &&
				lhs.parent == rhs.parent;
		}
		friend bool operator!=( const_tree_iterator const& lhs, const_tree_iterator const& rhs ) {
			return !( lhs == rhs );
		}*/

	};

}

namespace value_qtree
{
	template<typename TreeType, typename NodeIterType, typename ObjIterType> class node_iterator_base;
	template<typename TreeType> class node_iterator;
	template<typename TreeType> class const_node_iterator;


	template<typename TreeType> class tree_iterator {
	public:
		using tree_type = TreeType;
		using node_type = typename TreeType::node;
		using value_type = typename TreeType::value_type;
		using pointer = value_type*;
		using reference = value_type&;
		using node = node_type*;
		
	public:
		tree_iterator() = default;
		tree_iterator( 
			tree_type* tree_, 
			node node_,
			pointer value_ )
			:
			ptree( tree_ ),
			pnode( node_ ),
			pvalue( value_ )
		{}

		tree_iterator& operator++() {
			if( pvalue != value_end() ) {
				++pvalue;
				return *this;
			}

			// if pnode's parent is null, pnode is root and we are at the end
			if( pnode->m_pParent == nullptr ) {
				throw std::runtime_error( "cannot increment past end" );
			}

			pnode = find_sibling( pnode );
			pvalue = pnode->elements().data();
			return *this;
		}
		tree_iterator operator++( int ) {
			auto temp = *this;
			++( *this );
			return temp;
		}

		reference operator*() { return *pvalue; }
		pointer operator->() { return pvalue; }

		friend bool operator==( tree_iterator const& lhs, tree_iterator const& rhs ) {
			return ( lhs.pnode == rhs.pnode ) && ( lhs.pvalue == rhs.pvalue );
		}
		friend bool operator!=( tree_iterator const& lhs, tree_iterator const& rhs ) {
			return !( lhs == rhs );
		}
	private:
		node find_aunt( node current )const {
			
			// current is an only child ( is root )
			if( current->m_pParent == nullptr ) {
				return current;
			}

			auto mom_it = get_parent_iter( current->m_pParent );
			auto aunt_it = find_sibling( mom_it );
			return aunt_it;

			// // if mom is an only child, then current is root
			// if( aunt_it == mom_it ) {
			// 	return nullptr;
			// }
			// // if aunt is dead find grandma
			// auto gma_it = get_parent_iter( mom_it->get()->m_pParent );
			// if( aunt_it == gma_it->get()->m_pChildren.end() ) {
			// 	return find_aunt( gma_it );
			// }
			// else {
			// 	// aunt is alive
			// 	return ( *aunt_it ).get();
			// }
		}
		node get_parent_iter( node parent )const {
			auto it= std::find_if(
				parent->m_pParent->m_pChildren.begin(),
				parent->m_pParent->m_pChildren.end(),
				[&]( std::unique_ptr<node_type> const& child ) {return child.get() == parent; }
			);

			return it->get();
		}
		node find_min_child( node current )const {
			assert( current->m_pParent != nullptr );

			auto parent = get_parent_iter( current->m_pParent );
			auto& par_children = parent->m_pChildren;
			if( current == child_last(parent) ) {
				auto aunt = find_aunt( current );
				if( is_last_sibling(aunt) ) {
					return ( *parent )->m_pChildren.end();
				}


			}

			if( ( *current )->m_pChildren[ 0 ] ) {
				return find_min_child( ( *current )->m_pChildren.begin() );
			}

			return current;
		}

		/*
		* if current == root, returns current
		* if living sibling, returns sibling
		* if no living sibling, returns end
		*/
		node find_sibling( node current )const {
			// if no parent, current is root
			if( !( *current )->m_pParent ) {
				return current;
			}
			
			// find next living sibling
			auto* parent = ( *current )->m_pParent;
			auto sib_it = std::find_if(
				current + 1,
				parent->m_pChildren.end(),
				[]( std::unique_ptr<node_type> const& sib ) {return sib != nullptr; }
			);
			
			return sib_it;
			//// found a living sibling
			//if( sib_it != parent->m_pChildren.end() ) {
			//	return sib_it;
			//}
			//
			//// no living siblings 
			//// return end and check current's parent
			//return parent->m_pChildren.end();
		}
		bool is_last_sibling( node current )const {
			if( ( *current )->m_pParent == nullptr )
				return true;
			auto* parent = ( *current )->m_pParent;
			return current == parent->m_pChildren.end();
			std::find_if(
				parent->m_pChildren.begin(),
				parent->m_pChildren.end(),
				[&]( std::unique_ptr<node_type> const& child ) {return ( *current ) == child; }
			);

		}
		value_type const* value_end()const {
			return ( *pnode )->elements().data() + ( *pnode )->elements().size();
		}
		node child_last( node parent )const {
			return ( parent->m_pChildren.data() + int( parent->m_pChildren.size() ) - 1 )->get();
		}
	private:
		template<
			std::size_t allowed_objects_per_node,
			typename RectTraits,
			typename VecTraits,
			typename Object>
			friend class qtree;
		tree_type* ptree = nullptr;
		node pnode;
		pointer pvalue = nullptr;
		std::size_t current_parent_index = {};
	};

	template<typename TreeType> class const_tree_iterator {
	public:
		using tree_type = TreeType;
		using node_type = typename TreeType::node;
		using value_type = typename TreeType::value_type;
		using pointer = value_type const*;
		using reference = value_type const&;
		using node = node_type const*;

	public:
		const_tree_iterator() = default;
		const_tree_iterator(
			tree_type const* tree_,
			node node_,
			value_type const* value_,
			std::size_t root_index = {} )
			:
			ptree( tree_ ),
			pnode( node_ ),
			pvalue( value_ )
		{}

		const_tree_iterator& operator++() {
			if( pvalue != value_end() ) {
				++pvalue;
				return *this;
			}

			// if pnode's parent is null, pnode is root and we are at the end
			if( ( *pnode )->m_pParent == nullptr ) {
				throw std::runtime_error( "cannot increment past end" );
			}

			pnode = find_sibling( pnode );
			pvalue = ( *pnode )->elements().data();
			return *this;
		}
		const_tree_iterator operator++( int ) {
			auto temp = *this;
			++( *this );
			return temp;
		}

		reference operator*()const {
			return *pvalue;
		}
		pointer operator->()const {
			return pvalue;
		}
		friend bool operator==( const_tree_iterator const& lhs, const_tree_iterator const& rhs ) {
			return ( lhs.pnode == rhs.pnode ) && ( lhs.pvalue == rhs.pvalue );
		}
		friend bool operator!=( const_tree_iterator const& lhs, const_tree_iterator const& rhs ) {
			return !( lhs == rhs );
		}
	private:
		node find_aunt( node current )const {
			// current is an only child ( is root )
			if( current->get()->m_pParent == nullptr ) {
				return current;
			}

			auto mom_it = get_parent_iter( current->get()->m_pParent );
			auto aunt_it = find_sibling( mom_it );
			return aunt_it;

			// // if mom is an only child, then current is root
			// if( aunt_it == mom_it ) {
			// 	return nullptr;
			// }
			// // if aunt is dead find grandma
			// auto gma_it = get_parent_iter( mom_it->get()->m_pParent );
			// if( aunt_it == gma_it->get()->m_pChildren.end() ) {
			// 	return find_aunt( gma_it );
			// }
			// else {
			// 	// aunt is alive
			// 	return ( *aunt_it ).get();
			// }
		}
		node get_parent_iter( node parent )const {
			return std::find_if(
				parent->m_pParent->m_pChildren.begin(),
				parent->m_pParent->m_pChildren.end(),
				[&]( std::unique_ptr<node_type> const& child ) {return child.get() == parent; }
			);
		}
		node find_min_child( node current )const {
			assert( ( *current )->m_pParent != nullptr );

			auto parent = get_parent_iter( ( *current )->m_pParent );
			if( current == ( *parent )->m_pChildren.end() - 1 ) {
				auto aunt = find_aunt( current );
				if( is_last_sibling( aunt ) ) {
					return ( *parent )->m_pChildren.end();
				}


			}

			if( ( *current )->m_pChildren[ 0 ] ) {
				return find_min_child( ( *current )->m_pChildren.begin() );
			}

			return current;
		}

		/*
		* if current == root, returns current
		* if living sibling, returns sibling
		* if no living sibling, returns end
		*/
		node find_sibling( node current )const {
			// if no parent, current is root
			if( !( *current )->m_pParent ) {
				return current;
			}

			// find next living sibling
			auto* parent = ( *current )->m_pParent;
			auto sib_it = std::find_if(
				current + 1,
				parent->m_pChildren.end(),
				[]( std::unique_ptr<node_type> const& sib ) {return sib != nullptr; }
			);

			return sib_it;
			//// found a living sibling
			//if( sib_it != parent->m_pChildren.end() ) {
			//	return sib_it;
			//}
			//
			//// no living siblings 
			//// return end and check current's parent
			//return parent->m_pChildren.end();
		}
		bool is_last_sibling( node current )const {
			if( ( *current )->m_pParent == nullptr )
				return true;
			auto* parent = ( *current )->m_pParent;
			return current == parent->m_pChildren.end();
			std::find_if(
				parent->m_pChildren.begin(),
				parent->m_pChildren.end(),
				[&]( std::unique_ptr<node_type> const& child ) {return ( *current ) == child; }
			);

		}
		value_type const* value_end()const {
			return ( *pnode )->elements().data() + ( *pnode )->elements().size();
		}
	private:
		template<
			std::size_t allowed_objects_per_node,
			typename RectTraits,
			typename VecTraits,
			typename Object>
			friend class qtree;
		tree_type* ptree = nullptr;
		node pnode;
		pointer pvalue = nullptr;
		std::size_t current_parent_index = {};
	};

	template<
		std::size_t allowed_objects_per_node,
		typename RectTraits,
		typename VecTraits,
		typename Object>
		class qtree {
		public:
			using rect_traits = RectTraits;
			using vec_traits = VecTraits;

			using rect_type = typename rect_traits::rect_type;
			using vec_type = typename vec_traits::vector_type;

			using value_type = Object;
			using pointer = Object*;
			using reference = Object&;
			using const_pointer = Object const*;
			using const_reference = Object const&;

			using iterator = tree_iterator<qtree>;
			using const_iterator = const_tree_iterator<qtree>;
			static constexpr std::size_t max_objects = allowed_objects_per_node;
			

		public:
			class node {
			public:
				using array_iterator = typename std::array<std::unique_ptr<node>, 4>::iterator;
				using const_array_iterator = typename std::array<std::unique_ptr<node>, 4>::const_iterator;
			public:
				node( rect_type const& bounds_, node* pParent )
					:
					m_bounds( bounds_ ),
					m_pParent( pParent )
				{
					m_data.reserve( max_objects + 1 );
				}

				std::vector<value_type>& elements()noexcept {
					return m_data;
				}

				std::vector<value_type> const& elements()const noexcept {
					return m_data;
				}

				rect_type const& bounds()const noexcept {
					return m_bounds;
				}
			private:
				using find_result = std::pair<node*, typename std::vector<value_type>::iterator>;
				using const_find_result = std::pair<node const*, typename std::vector<value_type>::const_iterator>;

				void query( rect_type const& bounds, std::vector<pointer>& contained, qtree const& tree ) {
					if( !rect_traits::intersects( m_bounds, bounds ) ) return;

					for( auto& child : m_pChildren ) {
						if( !child ) continue;
						child->query( bounds, contained, tree );
					}

					if( m_data.empty() )return;

					contained.reserve( m_data.size() + contained.size() );

					std::transform(
						m_data.begin(),
						m_data.end(),
						std::back_inserter( contained ),
						[]( reference lhs ) { return &lhs; } 
					);
				}

				rect_type get_quadrant( int index )const noexcept {
					const auto left = rect_traits::left( m_bounds );
					const auto top = rect_traits::top( m_bounds );
					const auto right = rect_traits::right( m_bounds );
					const auto bottom = rect_traits::bottom( m_bounds );
					const auto center =
						rect_traits::template center<vec_traits>( m_bounds );

					switch( index ) {
						// left top
						case 0: return rect_traits::construct( left, top, center.x, center.y );
							// right top
						case 1: return rect_traits::construct( center.x, top, right, center.y );
							// left bottom 
						case 2: return rect_traits::construct( left, center.y, center.x, bottom );
							// right bottom
						case 3: return rect_traits::construct( center.x, center.y, right, bottom );
					}

					assert( index >= 0 && index < 4 && "Index out of range" );
					return rect_type{};
				}

				template<typename T>
				void add_to_child( qtree& tree, rect_type const& obj_bounds, T&& obj ) {
					using quadrant_result = std::optional<std::pair<int, rect_type>>;

					auto find_quadrant = [&]( rect_type const& obj_bounds )->quadrant_result
					{
						for( int index = 0; index < 4; ++index ) {
							auto quadrant = get_quadrant( index );
							if( rect_traits::contains( quadrant, obj_bounds ) ) {
								return quadrant_result{ std::in_place, index, quadrant };
							}
						}

						return quadrant_result{};
					};

					if( auto result = find_quadrant( obj_bounds ); !result.has_value() ) {
						m_data.push_back( std::move( obj ) );
					}
					else {
						auto [index, quadrant] = *result;
						auto& child = m_pChildren[ index ];

						if( !child ) {
							child = std::make_unique<node>( quadrant, this );
							tree.nodes.push_back( child.get() );
						}

						child->add_object( tree, obj_bounds, std::forward<T>( obj ) );
					}
				}

				template<typename T>
				void add_object( qtree& tree, rect_type const& obj_bounds, T&& obj ) {
					if( m_data.size() < max_objects ) {
						m_data.push_back( std::forward<T>( obj ) );
					}
					else {
						m_data.push_back( std::forward<T>( obj ) );
						auto temp = std::move( m_data );

						for( auto& obj : temp ) {
							add_to_child( tree, tree.get_rect( obj ), std::move( obj ) );
						}
					}
				}

				find_result find_object( const_reference object, rect_type const& obj_bounds ) {
					auto is_same = [&]( const_reference data_ ) { return &data_ == &object; };
					auto it = std::find_if( m_data.begin(), m_data.end(), is_same );

					if( it != m_data.end() ) return { this, it };

					for( int index = 0; auto & child : m_pChildren ) {
						if( !child ) continue;
						if( !rect_traits::intersects( child->bounds(), obj_bounds ) )continue;

						auto result = child->find_object( object, obj_bounds );
						if( result.first != nullptr ) return result;
					}

					return { nullptr, m_data.end() };
				}

				const_find_result find_object( const_reference object, rect_type const& obj_bounds )const {
					auto is_same = [&]( const_reference data_ ) { return &data_ == &object; };
					auto it = std::find_if( m_data.begin(), m_data.end(), is_same );

					if( it != m_data.end() ) return { this, it };

					for( int index = 0; auto & child : m_pChildren ) {
						if( !child ) continue;
						if( !rect_traits::intersects( child->bounds(), obj_bounds ) )continue;

						auto result = child->find_object( object, obj_bounds );
						if( result.first != nullptr ) return result;
					}

					return { nullptr, m_data.end() };
				}

				node* find_node( rect_type const& obj_bounds ) {
					for( auto& child : m_pChildren ) {
						if( !child ) continue;
						if( !rect_traits::intersects( child->bounds(), obj_bounds ) ) continue;
						return child->find_node( obj_bounds );
					}

					return this;
				}

				node const* find_node( rect_type const& obj_bounds )const {
					for( auto& child : m_pChildren ) {
						if( !child ) continue;
						if( !rect_traits::intersects( child->bounds(), obj_bounds ) ) continue;
						return child->find_node( obj_bounds );
					}

					return this;
				}

				node* find_last_node( node* current ) {
					for( auto i = std::size_t{}; i < std::size_t{ 4 }; ++i ) {
						const auto index = std::size_t{ 3 } - i;
						if( current->m_pChildren[ index ] ) {
							return find_last_node( current->m_pChildren[ index ].get() );
						}
					}

					return this;
				}

				array_iterator find_min_node(qtree& ptree ) {
					if( this != &ptree.root ) {
						throw std::invalid_argument( "only callable from qtree::root" );
					}

					if( is_leaf() ) {
						throw std::runtime_error( "tree is empty" );
					}

					return find_min_node( m_pChildren.begin() );
				}
				array_iterator find_min_node( array_iterator cur ) {

					// empty iterator...return empty iterator
					if( cur == array_iterator{} ) {
						return array_iterator{};
					}

					// end iterator...return cur
					if( cur != ( *cur )->m_pChildren.end() ) {
						return find_min_node( ( *cur )->m_pChildren.begin() );
					}
					
					return cur;
					
				}
				const_array_iterator find_min_node( qtree const& ptree )const {
					if( this != &ptree.root ) {
						throw std::invalid_argument( "only callable from qtree::root" );
					}

					if( is_leaf() ) {
						throw std::runtime_error( "tree is empty" );
					}

					return find_min_node( m_pChildren.begin() );
				}
				const_array_iterator find_min_node( const_array_iterator cur )const {

					// empty iterator...return empty iterator
					if( cur == const_array_iterator{} ) {
						return const_array_iterator{};
					}

					// end iterator...return cur
					if( cur != ( *cur )->m_pChildren.end() ) {
						return find_min_node( ( *cur )->m_pChildren.begin() );
					}

					return cur;

				}
				auto is_leaf()const {
					for( auto& child : m_pChildren ) {
						if( child )return false;
					}
					return true;
				}
			private:
				template<typename> friend class tree_iterator;

				friend class qtree;
				std::array<std::unique_ptr<node>, 4> m_pChildren;
				std::vector<value_type> m_data;
				rect_type m_bounds;
				node* m_pParent = nullptr;
			};

			qtree( rect_type const& bounds_, std::function<rect_type( value_type const& )> get_rect_fn )
				:
				root( bounds_, nullptr ),
				get_rect( std::move( get_rect_fn ) )
			{
				nodes.reserve( 256 );
				nodes.push_back( &root );
			}

			void push( value_type const& object ) {
				root.add_object( *this, get_rect( object ), object );
				++count;
			}
			void push( value_type&& object ) {
				root.add_object( *this, get_rect(object), std::move( object ) );
				++count;
			}

			template<typename...Args>
			void emplace( Args&&... args ) {
				auto obj = value_type{ std::forward<Args>( args )... };
				root.add_object( *this, get_rect( obj ), std::move( obj ) );
				++count;
			}

			void clear()noexcept {
				root = node( root.m_bounds, nullptr );
				count = std::size_t{};
			}

			iterator begin()noexcept {
				auto beg = root.find_min_node( *this );

				return iterator( this, beg, (*beg)->elements().data() );
			}
			iterator end()noexcept {
				auto it = typename node::array_iterator{};
				return iterator( this, it, root.m_data.data() );
			}

			const_iterator begin()const noexcept {
				auto beg = root.find_min_node( *this );
				return const_iterator( this, beg, ( *beg )->elements().data() );
			}
			const_iterator end()const noexcept {
				auto it = typename node::array_iterator{};
				return const_iterator( this, it, root.m_data.data() );
			}

			std::vector<pointer> query( rect_type const& bounds ) {
				std::vector<pointer> objects;
				root.query( bounds, objects, *this );
				return objects;
			}

			iterator erase( const_iterator where ) {
				//const auto& self = *this;
				//if( where == self.end() ) {
				//	throw std::runtime_error( "cannot delete end iterator" );
				//}
				//auto pobj_dist = where.pvalue - ( *where.pnode )->elements().data();
				//auto& elements = ( *where.pnode )->elements();
				//auto obj_it = elements.erase( elements.begin() + pobj_dist );
				//auto obj_it = ( *where.current_node )->elements().erase( where.it );

				// Remove node from tree if it is an empty leaf node
				//auto get_child_index = [&]() {
				//	auto index = std::size_t{};
				//
				//	for( auto & child : ( *where.current_node )->m_pParent->m_pChildren ) {
				//		if( child.get() != ( *where.current_node ) ) {
				//			++index;
				//		}
				//		else {
				//			break;
				//		}
				//	}
				//
				//	return index;
				//};
				//if( ( *where.current_node )->is_leaf() &&
				//	( *where.current_node )->elements().empty() ) {
				//	if( auto index = get_child_index(); index < 4 ) {
				//		( *where.current_node )->m_pParent->m_pChildren[ index ] = 
				//			std::unique_ptr<node>{};
				//		where.current_node = nodes.erase( where.current_node );
				//		if( where.current_node != nodes.end() ) {
				//			obj_it = ( *where.current_node )->m_pParent->elements().begin();
				//		}
				//	}
				//}
				//
				//auto dist = std::distance( self.nodes.begin(), where.current_node );
				//auto node_it = nodes.begin() + dist;
				//
				//--count;
				//
				//// Equal operator expects ( nodes.end() - 1 ) for end
				//if( node_it == nodes.end() ) {
				//	return end();
				//}
				//
				//// Wrap obj_it to the beginning of the next node 
				//// if at the end of current node
				//if( obj_it == ( *node_it )->elements().end() ) {
				//	++node_it;
				//	obj_it = ( *node_it )->elements().begin();
				//}

				//return iterator{ this, node_it, obj_it };
				return begin();
			}

			iterator erase( const_iterator from, const_iterator to ) {
				// auto it = from;
				// while( it != to ) {
				// 	it = erase( it );
				// }
				// 
				// return convert( it );
				return begin();
			}

			auto remove_object( const_reference object ) {
				auto const& self = *this;
				return erase( self.find_object( object ) );
			}

			iterator find_object( const_reference object ) {
				//auto [n, obj_it] = root.find_object( object, get_rect( object ) );
				//
				//if( n == nullptr ) return end();
				//
				//auto node_it =
				//	std::find( nodes.begin(), nodes.end(), n );
				//return { this, node_it, obj_it };
				return begin();
			}
	
			const_iterator find_object( const_reference object )const {
				//auto [n, obj_it] = root.find_object( object, get_rect( object ) );
				//
				//if( n == nullptr ) return end();
				//
				//auto node_it =
				//	std::find( nodes.begin(), nodes.end(), n );
				//return { this, node_it, obj_it };
				return begin();
			}

			auto size()const noexcept {
				return count;
			}

		private:
			iterator convert( const_iterator citer ) noexcept {
				//auto node_dist = std::distance( nodes.cbegin(), citer.current_node );
				//auto obj_dist = std::distance( ( *citer.current_node )->elements().cbegin(), citer.it );
				//auto node_iter = nodes.begin() + node_dist;
				//auto obj_iter = ( *node_iter )->elements().begin() + obj_dist;
				//
				//return { this, node_iter, obj_iter };
				return begin();
			}
			const_iterator convert( iterator iter )noexcept {
				//auto node_dist = std::distance( nodes.begin(), iter.current_node );
				//auto obj_dist = std::distance( ( *iter.current_node )->elements().begin(), iter.it );
				//auto node_iter = nodes.cbegin() + node_dist;
				//auto obj_iter = ( *node_iter )->elements().cbegin() + obj_dist;
				//
				//return { this, node_iter, obj_iter };
				const auto& self = *this;
				return self.begin();
			}
			//node* find_last_node()noexcept {
			//	node* c = &root;
			//	const node* copy = c;
			//	node* n = nullptr;
			//	for( auto i = std::size_t{ 4 }; i > 0; --i ) {
			//		const auto index = i - std::size_t{ 1 };
			//		do
			//		{
			//			n = n->m_pChildren[ index ].get();
			//			if( n != nullptr ) c = n;
			//		} while( n );
			//
			//		if( c != copy ) {
			//			break;
			//		}
			//	}
			//
			//	return c;
			//}
		private:
			template<typename TreeType, typename NodeIterType, typename ObjIterType> friend class node_iterator_base;
			template<typename TreeType> friend class node_iterator;
			template<typename TreeType> friend class const_node_iterator;
			template<typename> friend class tree_iterator;

			node root;
			std::function<rect_type( const value_type& )> get_rect;
			std::vector<node*> nodes;
			std::size_t count = std::size_t{};
	};

	template<typename Iter> struct is_const_iter {
		using type = std::remove_reference_t<typename Iter::reference>;
		static constexpr bool value = std::is_const_v<type>;
	};

	template<typename TreeType, typename NodeIterType, typename ObjIterType>
	class node_iterator_base {
		using tree_ptr = std::conditional_t<is_const_iter<ObjIterType>::value, TreeType const*, TreeType* > ;

		using node_iter_type = NodeIterType;
		using iter_type = ObjIterType;

		using rect_traits = typename TreeType::rect_traits;
		using rect_type = typename rect_traits::rect_type;

	public:
		node_iterator_base( tree_ptr ptree_, node_iter_type node_it, iter_type obj_it )
			:
			ptree( ptree_ ),
			current_node( node_it ),
			it( obj_it )
		{}
		rect_type bounds()const noexcept {
			return ( *current_node )->bounds();
		}
		
	protected:
		auto find_prev_not_empty_node( node_iter_type node_it )const {
			do {
				if( node_it == ptree->nodes.begin() ) break;
				--node_it;
			} while( ( *node_it )->elements().empty() );

			return node_it;
		};
		auto find_next_not_empty_node( node_iter_type node_it )const {
			do {
				if( node_it == ptree->nodes.end() - 1 ) break;
				++node_it;
			} while( ( *node_it )->elements().empty() );
			return node_it;
		};
		void increment() {
			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "increment would be out of range" );
			}

			++it;
			if( it == ( *current_node )->elements().end() ) {
				current_node = find_next_not_empty_node( current_node );

				if( current_node != ( ptree->nodes.end() - 1 ) ) {
					it = ( *current_node )->elements().begin();
				}
				else {
					it = ( *current_node )->elements().end();
				}
			}
		}
		void decrement() {
			if( current_node == ptree->nodes.begin() ) {
				throw std::runtime_error( "decrement would be out of range" );
			}
			else if( current_node == ptree->nodes.end() ) {
				current_node = find_prev_not_empty_node( current_node );
				it = ( *current_node )->elements().end();
			}

			if( it != ( *current_node )->elements().begin() ) {
				--it;
			}
			else {
				current_node = find_prev_not_empty_node( current_node );

				if( ( *current_node )->elements().empty() ) {
					it = ( *current_node )->elements().begin();
				}
				else {
					it = ( *current_node )->elements().end() - 1;
				}
			}
		}
	protected:
		template<
			std::size_t allowed_objects_per_node,
			typename RectTraits,
			typename VecTraits,
			typename Object>
			friend class qtree;
		tree_ptr ptree = nullptr;
		node_iter_type current_node;
		iter_type it;
	};

	template<typename TreeType>
	class node_iterator : public node_iterator_base<
		TreeType, 
		typename std::vector<typename TreeType::node*>::iterator,
		typename std::vector<typename TreeType::value_type>::iterator
	>{
	public:
		using value_type = typename TreeType::value_type;
		using reference = typename TreeType::reference;
		using pointer = typename TreeType::pointer;
		using node_iter_type = decltype( std::declval<TreeType>().nodes.begin() );
		using iter_type = decltype( std::declval<typename TreeType::node>().elements().begin() );
		using iterator_category = std::bidirectional_iterator_tag;
		using difference_type = std::ptrdiff_t;
		using base = node_iterator_base<TreeType, node_iter_type, iter_type>;
	public:
		node_iterator() = default;
		node_iterator( TreeType* ptree_, node_iter_type node_it, iter_type obj_it )
			:
			base( ptree_, node_it, obj_it )
		{}

		operator const_node_iterator<TreeType>()const {
			return{ ptree, current_node, it };
		}

		node_iterator& operator++() {
			increment();
			return *this;
		}
		node_iterator operator++( int ) {
			auto copy = *this;
			++( *this );
			return copy;
		}
		node_iterator& operator--() {
			decrement();
			return *this;
		}
		node_iterator operator--( int ) {
			auto copy = *this;
			--( *this );
			return copy;
		}

		reference operator*() {
			if( current_node == ptree->nodes.end() ||
				it == (*current_node)->elements().end() ) {
				throw std::runtime_error( "cannot dereference end" );
			}

			return *it;
		}
		pointer operator->() {
			if( current_node == ptree->nodes.end() ||
				it == ( *current_node )->elements().end() ) {
				throw std::runtime_error( "cannot dereference end" );
			}

			return &( *it );
		}

		friend bool operator==( node_iterator const& lhs, node_iterator const rhs ) {
			return ( lhs.current_node == rhs.current_node ) && ( lhs.it == rhs.it );
		}

		friend bool operator!=( node_iterator const& lhs, node_iterator const rhs ) {
			return !( lhs == rhs );
		}
	};
	
	template<typename TreeType>
	class const_node_iterator : 
		public node_iterator_base<
		TreeType,
		typename std::vector<typename TreeType::node*>::const_iterator,
		typename std::vector<typename TreeType::value_type>::const_iterator
		> {
	public:
		using value_type = typename TreeType::value_type;
		using reference = typename TreeType::const_reference;
		using pointer = typename TreeType::const_pointer;
		using node_iter_type = decltype( std::declval<TreeType>().nodes.cbegin() );
		using iter_type = decltype( std::declval<typename TreeType::node>().elements().cbegin() );
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::bidirectional_iterator_tag;
		using base = node_iterator_base<TreeType, node_iter_type, iter_type>;

	public:
		const_node_iterator() = default;
		const_node_iterator( TreeType const* ptree_, node_iter_type node_it, iter_type obj_it )
			:
			base( ptree_, node_it, obj_it )
		{}

		const_node_iterator& operator++() {
			increment();
			return *this;
		}
		const_node_iterator operator++( int ) {
			auto copy = *this;
			++( *this );
			return copy;
		}
		const_node_iterator& operator--() {
			decrement();
			return *this;
		}
		const_node_iterator operator--( int ) {
			auto copy = *this;
			--( *this );
			return copy;
		}

		reference operator*()const {
			if( current_node == ptree->nodes.end() ||
				it == ( *current_node )->elements().end() ) {
				throw std::runtime_error( "cannot dereference end" );
			}

			return *it;
		}
		pointer operator->()const {
			if( current_node == ptree->nodes.end() ||
				it == ( *current_node )->elements().end() ) {
				throw std::runtime_error( "cannot dereference end" );
			}

			return &( *it );
		}

		friend bool operator==( const_node_iterator const& lhs, const_node_iterator  const rhs ) {
			return ( lhs.current_node == rhs.current_node ) && ( lhs.it == rhs.it );
		}

		friend bool operator!=( const_node_iterator const& lhs, const_node_iterator  const rhs ) {
			return !( lhs == rhs );
		}
	};

	
}