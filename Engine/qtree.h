#pragma once

#include "rect_traits.h"
#include "vector_traits.h"
#include <array>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

template<typename T>
void swap_and_pop( std::vector<T>& vec, typename std::vector<T>::iterator iter ) {
	if( vec.empty() )return;
	if( vec.size() == std::size_t( 1 ) )vec.pop_back();
	
	*iter = vec.back();
	vec.pop_back();
}

namespace primary
{
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
}


namespace value_qtree
{
	template<typename TreeType, typename ValueT, typename PointerT, typename ReferenceT>
	class tree_iterator_base;
	template<typename TreeType> class tree_iterator;
	template<typename TreeType> class const_tree_iterator;
	template<typename TreeType> class node_iterator;
	template<typename TreeType> class const_node_iterator;

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

			using iterator = node_iterator<qtree>;
			using const_iterator = const_node_iterator<qtree>;
			static constexpr std::size_t max_objects = allowed_objects_per_node;
			

		public:
			class node {
			public:
				node( rect_type const& bounds_, node* pParent )
					:
					m_bounds( bounds_ ),
					m_pParent( pParent )
				{
					m_data.reserve( max_objects );
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

					for( auto& element : m_data ) {
						if( rect_traits::intersects( tree.get_rect( element ), bounds ) )
							contained.push_back( &element );
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

				bool add_to_child( const_reference object, qtree& tree ) {
					if( m_data.size() >= max_objects ) return false;

					for( int index = 0; auto & child : m_pChildren ) {
						auto quadrant = get_quadrant( index++ );

						if( !rect_traits::contains( quadrant, tree.get_rect( object ) ) ) continue;
						if( !child ) {
							child = std::make_unique<node>( quadrant, this );
							tree.nodes.push_back( child.get() );
						}
						child->add_object( object, tree );
						return true;
					}

					return false;
				}

				void add_object( const_reference object, qtree& tree ) {
					if( !add_to_child( object, tree ) ) {
						m_data.push_back( object );
						++tree.count;
					}
				}

				bool add_to_child( value_type&& object, qtree& tree ) {
					if( m_data.size() >= max_objects ) return false;

					for( int index = 0; auto & child : m_pChildren ) {
						auto quadrant = get_quadrant( index++ );

						if( !rect_traits::contains( quadrant, tree.get_rect( object ) ) ) continue;
						if( !child ) 
						{
							child = std::make_unique<node>( quadrant, this );
							tree.nodes.push_back( child.get() );
						}
						child->add_object( std::move( object ), tree );
						return true;
					}

					return false;
				}

				void add_object( value_type&& object, qtree& tree ) {
					if( !add_to_child( std::move( object ), tree ) ) {
						m_data.emplace_back( std::move( object ) );
						++tree.count;
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
						auto result = child->find_node( obj_bounds );
						if( result )return result;
					}

					return this;
				}

				auto is_leaf()const {
					for( auto& child : m_pChildren ) {
						if( child )return false;
					}
					return true;
				}
			private:
				friend class qtree;
				
				template<typename TreeType, typename ValueT, typename PointerT, typename ReferenceT>
				friend class tree_iterator_base;
				
				template<typename TreeType> 
				friend class tree_iterator;
				
				template<typename TreeType> 
				friend class const_tree_iterator;

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
				root.add_object( object, *this );
			}
			void push( value_type&& object ) {
				root.add_object( std::move( object ), *this );
			}

			template<typename...Args>
			void emplace( Args&&... args ) {
				root.add_object( value_type{ std::forward<Args>( args )... }, *this);
			}

			void clear()noexcept {
				root = node( root.m_bounds, nullptr );
			}

			iterator begin()noexcept {
				return iterator( this, nodes.begin(), (*nodes.begin())->m_data.begin() );
			}
			iterator end()noexcept {
				auto end_node = nodes.end() - 1;
				return iterator( this, nodes.end(), (*end_node )->m_data.end() );
			}

			const_iterator begin()const noexcept {
				return const_iterator( this, nodes.begin(), ( *nodes.begin() )->m_data.begin() );
			}
			const_iterator end()const noexcept {
				auto end_node = nodes.end() - 1;
				return const_iterator( this, nodes.end(), ( *end_node )->m_data.end() );
			}

			std::vector<pointer> query( rect_type const& bounds ) {
				std::vector<pointer> objects;
				root.query( bounds, objects, *this );
				return objects;
			}

			iterator erase( const_iterator where ) {
				const auto& self = *this;
				if( where == self.end() ) {
					throw std::runtime_error( "cannot delete end iterator" );
				}

				auto it = ( *where.current_node )->elements().erase( where.it );

				if( ( *where.current_node )->is_leaf() &&
					( *where.current_node )->elements().empty() ) {
					auto parent = ( *where.current_node )->m_pParent;
					for( auto& child : parent->m_pChildren ) {
						if( child.get() == ( *where.current_node ) ) {
							where.current_node = nodes.erase( where.current_node );
							child = std::unique_ptr<node>{};
						}
					}
				}

				auto dist = std::distance( self.nodes.begin(), where.current_node );
				auto node_it = nodes.begin() + dist;
				--count;
				return iterator{ this, node_it, it };
			}

			iterator erase( const_iterator from, const_iterator to ) {
				auto it = from;
				while( it != to ) {
					it = convert( erase( it ) );
				}
				
				return convert( it );
			}

			auto remove_object( const_reference object ) {
				return erase( convert( find_object( object ) ) );
			}

			iterator find_object( const_reference object ) {
				auto [n, obj_it] = root.find_object( object, get_rect( object ) );

				if( n == nullptr ) return end();

				auto node_it =
					std::find( nodes.begin(), nodes.end(), n );
				return { this, node_it, obj_it };
			}
	
			const_iterator find_object( const_reference object )const {
				auto [n, obj_it] = root.find_object( object, get_rect( object ) );

				if( n == nullptr ) return end();

				auto node_it =
					std::find( nodes.begin(), nodes.end(), n );
				return { this, node_it, obj_it };
			}
			auto size()const noexcept {
				return count;
			}

		private:
			iterator convert( const_iterator citer ) noexcept {
				auto node_dist = std::distance( nodes.cbegin(), citer.current_node );
				auto obj_dist = std::distance( ( *citer.current_node )->elements().cbegin(), citer.it );
				auto node_iter = nodes.begin() + node_dist;
				auto obj_iter = ( *node_iter )->elements().begin() + obj_dist;

				return { this, node_iter, obj_iter };
			}
			const_iterator convert( iterator iter )noexcept {
				auto node_dist = std::distance( nodes.begin(), iter.current_node );
				auto obj_dist = std::distance( ( *iter.current_node )->elements().begin(), iter.it );
				auto node_iter = nodes.cbegin() + node_dist;
				auto obj_iter = ( *node_iter )->elements().cbegin() + obj_dist;

				return { this, node_iter, obj_iter };
			}
		private:
			template<typename TreeType, typename ValueT, typename PointerT, typename ReferenceT>
			friend class tree_iterator_base;
			template<typename TreeType> friend class tree_iterator;
			template<typename TreeType> friend class const_tree_iterator;
			template<typename TreeType> friend class node_iterator;
			template<typename TreeType> friend class const_node_iterator;

			node root;
			std::function<rect_type( const value_type& )> get_rect;
			std::vector<node*> nodes;
			std::size_t count = std::size_t{};
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
			for( auto & child : parent_->m_pChildren ) {
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
		friend bool operator==( tree_iterator const& lhs, tree_iterator const& rhs ) {
			return
				lhs.current == rhs.current &&
				lhs.parent == rhs.parent;
		}
		friend bool operator!=( tree_iterator const& lhs, tree_iterator const& rhs ) {
			return !( lhs == rhs );
		}

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

		friend bool operator==( const_tree_iterator const& lhs, const_tree_iterator const& rhs ) {
			return
				lhs.current == rhs.current &&
				lhs.parent == rhs.parent;
		}
		friend bool operator!=( const_tree_iterator const& lhs, const_tree_iterator const& rhs ) {
			return !( lhs == rhs );
		}

	};

	template<typename TreeType>
	class node_iterator {
	public:
		using value_type = typename TreeType::value_type;
		using reference = typename TreeType::reference;
		using pointer = typename TreeType::pointer;
		using node_iter_type = decltype( std::declval<TreeType>().nodes.begin() );
		using iter_type = decltype( std::declval<typename TreeType::node>().elements().begin() );

	public:
		node_iterator() = default;
		node_iterator( TreeType* ptree_, node_iter_type node_it, iter_type obj_it )
			:
			ptree( ptree_ ),
			current_node( node_it ),
			it( obj_it )
		{}

		node_iterator& operator++() {
			auto find_next_not_empty_node = [&]( node_iter_type node_it ) {
				do {
					++node_it;
				} while( node_it != ptree->nodes.end() &&
					( *node_it )->elements().empty() );

				return node_it;
			};

			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "increment would be out of range" );
			}

			++it;
			if( it == ( *current_node )->elements().end() ) {
				current_node = find_next_not_empty_node( current_node );

				if( current_node != ptree->nodes.end() ) {
					it = ( *current_node )->elements().begin();
				}
			}

			return *this;
		}
		node_iterator operator++( int ) {
			auto copy = *this;
			++( *this );
			return copy;
		}
		node_iterator& operator--() {
			auto find_prev_not_empty_node = [&]( node_iter_type node_it ) {
				do {
					--node_it;
				} while( ( *node_it )->elements().empty() &&
					node_it != ptree->nodes.begin() );

				return node_it;
			};
			if( current_node == ptree->nodes.begin() ) {
				throw std::runtime_error( "decrement would be out of range" );
			}
			else if( current_node == ptree->nodes.end() ) {
				current_node = find_prev_not_empty_node( current_node );
				auto* node = ( *current_node );
				it = node->elements().end();
			}
			
			if( it != ( *current_node )->elements().begin() ) {
				--it;
			}
			else {
				current_node = find_prev_not_empty_node( current_node );

				// test since current_node could be == to ptree->nodes.begin()
				if( ( *current_node )->elements().empty() ) {
					it = ( *current_node )->elements().begin();
				}
				else {
					it = ( *current_node )->elements().end() - 1;
				}
			}
			return *this;
		}
		node_iterator operator--( int ) {
			auto copy = *this;
			--( *this );
			return copy;
		}

		reference operator*() {
			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "cannot dereference end" );
			}

			return *it;
		}
		pointer operator->() {
			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "cannot dereference end" );
			}

			return &( *it );
		}

		friend bool operator==( node_iterator const& lhs, node_iterator const rhs ) {
			return 
				( lhs.current_node == rhs.current_node ) &&
				( lhs.it == rhs.it );
		}

		friend bool operator!=( node_iterator const& lhs, node_iterator const rhs ) {
			return !( lhs == rhs );
		}
	private:
		friend class TreeType;
		TreeType* ptree = nullptr;
		node_iter_type current_node;
		iter_type it;
	};
	
	template<typename TreeType>
	class const_node_iterator {
	public:
		using value_type = typename TreeType::value_type;
		using reference = typename TreeType::const_reference;
		using pointer = typename TreeType::const_pointer;
		using node_iter_type = decltype( std::declval<TreeType>().nodes.cbegin() );
		using iter_type = decltype( std::declval<typename TreeType::node>().elements().cbegin() );

	public:
		const_node_iterator() = default;
		const_node_iterator( TreeType const* ptree_, node_iter_type node_it, iter_type obj_it )
			:
			ptree( ptree_ ),
			current_node( node_it ),
			it( obj_it )
		{}

		const_node_iterator& operator++() {
			auto find_next_not_empty_node = [&]( node_iter_type node_it ) {
				do {
					++node_it;
				} while( node_it != ptree->nodes.end() &&
					( *node_it )->elements().empty() );

				return node_it;
			};

			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "increment would be out of range" );
			}

			++it;
			if( it == ( *current_node )->elements().end() ) {
				current_node = find_next_not_empty_node( current_node );

				if( current_node != ptree->nodes.end() ) {
					it = ( *current_node )->elements().begin();
				}
			}

			return *this;
		}
		const_node_iterator operator++( int ) {
			auto copy = *this;
			++( *this );
			return copy;
		}
		const_node_iterator& operator--() {
			auto find_prev_not_empty_node = [&]( node_iter_type node_it ) {
				do {
					--node_it;
				} while( ( *node_it )->elements().empty() &&
					node_it != ptree->nodes.begin() );

				return node_it;
			};
			if( current_node == ptree->nodes.begin() ) {
				throw std::runtime_error( "decrement would be out of range" );
			}
			else if( current_node == ptree->nodes.end() ) {
				current_node = find_prev_not_empty_node( current_node );
				auto* node = ( *current_node );
				it = node->elements().end();
			}

			if( it != ( *current_node )->elements().begin() ) {
				--it;
			}
			else {
				current_node = find_prev_not_empty_node( current_node );

				// test since current_node could be == to ptree->nodes.begin()
				if( ( *current_node )->elements().empty() ) {
					it = ( *current_node )->elements().begin();
				}
				else {
					it = ( *current_node )->elements().end() - 1;
				}
			}
			return *this;
		}
		const_node_iterator operator--( int ) {
			auto copy = *this;
			--( *this );
			return copy;
		}

		reference operator*()const {
			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "increment past end" );
			}

			return *it;
		}
		pointer operator->()const {
			if( current_node == ptree->nodes.end() ) {
				throw std::runtime_error( "increment past end" );
			}

			return &( *it );
		}

		friend bool operator==( const_node_iterator  const& lhs, const_node_iterator  const rhs ) {
			return lhs.it == rhs.it;
		}

		friend bool operator!=( const_node_iterator  const& lhs, const_node_iterator  const rhs ) {
			return lhs.it != rhs.it;
		}
	private:
		friend class TreeType;
		TreeType const* ptree = nullptr;
		node_iter_type current_node = {};
		iter_type it = {};
	};

	
}