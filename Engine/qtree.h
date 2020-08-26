#pragma once

#include "rect_traits.h"
#include "vector_traits.h"
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <vector>


template<typename T, typename RectTraits> class node_data {
public:
	using rect_traits = RectTraits;
	using rect_type = typename RectTraits::rect_type;

public:
	node_data() = default;
	node_data( std::size_t id, rect_type const& bounds )
		: m_bounds( bounds ), m_id( id ), m_init( true ) {}

	operator bool()const { return m_init; }

	rect_type const& bounds()const { return m_bounds; }

	bool contains( rect_type const& rhs )const {
		return rect_traits::contains( m_bounds, rhs );
	}

	bool intersects( rect_type const& rhs )const {
		return rect_traits::intersects( m_bounds, rhs );
	}

	auto& elements() { return m_objects; }
	auto& elements()const { return m_objects; }

	std::size_t count()const noexcept {
		return m_objects.size();
	}

	bool empty()const noexcept {
		return m_objects.empty();
	}
	void visit() { m_query_visit = true; }
	bool visited()const { return m_query_visit; }
private:
	template<std::size_t, typename, typename, typename> friend class qtree;

	rect_type m_bounds;
	std::vector<T> m_objects;
	std::size_t m_id = {};
	bool m_init = false;
	bool m_query_visit = false;
};

template<
	std::size_t allowed_objects_per_node,
	typename RectTraits,
	typename VecTraits,
	typename Object>
	class qtree {
	public:
		using value_type = Object;
		using pointer = Object*;
		using reference = Object&;
		using const_pointer = Object const*;
		using const_reference = Object const&;

		using rect_traits = RectTraits;
		using vec_traits = VecTraits;

		using rect_type = typename rect_traits::rect_type;
		using vec_type = typename vec_traits::vector_type;

		using node_type = node_data<value_type, rect_traits>;

		static constexpr std::size_t max_objects = allowed_objects_per_node;

	public:
		class iterator {
			using node_iterator = typename std::vector<node_type>::iterator;
			using value_type = node_type;
			using pointer = value_type*;
			using reference = value_type&;
			using difference_type = std::ptrdiff_t;
			using iterator_category = std::bidirectional_iterator_tag;

		public:
			iterator() = default;
			iterator( node_iterator it, qtree* parent )
				:
				m_iter( it ),
				m_parent( parent )
			{}

			iterator& operator++() {
				do {
					++m_iter;
				} while( m_iter != m_parent->m_nodes.end() && !( *m_iter ) );

				return *this;
			}
			iterator operator++( int ) {
				auto copy = *this;
				++( *this );
				return copy;
			}
			iterator& operator--() {
				if( m_iter != m_parent->m_nodes.begin() ) {
					do {
						--m_iter;
					} while( m_iter != m_parent->m_nodes.begin() && !( *m_iter ) );
				}

				return *this;
			}
			iterator operator--( int ) {
				auto copy = *this;
				--( *this );
				return copy;
			}

			reference operator*() {
				if( m_iter == m_parent->m_nodes.end() ) {
					throw std::runtime_error( "cannot dereferrence end" );
				}

				return *m_iter;
			}
			pointer operator->() {
				if( m_iter == m_parent->m_nodes.end() ) {
					throw std::runtime_error( "cannot dereferrence end" );
				}

				return std::addressof( *m_iter );
			}

			bool operator==( iterator const& rhs )const noexcept {
				return m_iter == rhs.m_iter;
			}
			bool operator!=( iterator const& rhs )const noexcept {
				return !( *this == rhs );
			}
		private:
			friend class qtree;
			node_iterator m_iter;
			qtree* m_parent = nullptr;
		};

		class const_iterator
		{
			using node_iterator = typename std::vector<node_type>::const_iterator;
			using value_type = node_type;
			using pointer = value_type const*;
			using reference = value_type const&;
			using difference_type = std::ptrdiff_t;
			using iterator_category = std::bidirectional_iterator_tag;

		public:
			const_iterator() = default;
			const_iterator( node_iterator it, qtree const* parent )
				:
				m_iter( it ),
				m_parent( parent )
			{}

			const_iterator& operator++() {
				do {
					++m_iter;
				} while( m_iter != m_parent->m_nodes.end() && !( *m_iter ) );

				return *this;
			}
			const_iterator operator++( int ) {
				auto copy = *this;
				++( *this );
				return copy;
			}
			const_iterator& operator--() {
				if( m_iter != m_parent->m_nodes.begin() ) {
					do {
						--m_iter;
					} while( m_iter != m_parent->m_nodes.begin() && !( *m_iter ) );
				}

				return *this;
			}
			const_iterator operator--( int ) {
				auto copy = *this;
				--( *this );
				return copy;
			}

			reference operator*()const {
				if( m_iter == m_parent->m_nodes.end() ) {
					throw std::runtime_error( "cannot dereferrence end" );
				}

				return *m_iter;
			}
			pointer operator->()const {
				if( m_iter == m_parent->m_nodes.end() ) {
					throw std::runtime_error( "cannot dereferrence end" );
				}

				return std::addressof( *m_iter );
			}

			bool operator==( const_iterator const& rhs )const noexcept {
				return m_iter == rhs.m_iter;
			}
			bool operator!=( const_iterator const& rhs )const noexcept {
				return !( *this == rhs );
			}
		private:
			friend class qtree;
			node_iterator m_iter;
			qtree const* m_parent = nullptr;
		};

	public:
		qtree( rect_type const& bounds_, std::function<rect_type( value_type const& )> get_rect_fn )
			:
			m_get_rect( std::move( get_rect_fn ) )
		{
			m_nodes.push_back( { std::size_t{}, bounds_ } );
		}

		void push( value_type const& object ) {
			add_data( m_get_rect( object ), object );
			++m_count;
		}
		void push( value_type&& object ) {
			add_data( m_get_rect( object ), std::move( object ) );
			++m_count;
		}

		template<typename...Args>
		void emplace( Args&&... args ) {
			push( { std::forward<Args>( args )... } );
			++m_count;
		}

		iterator begin() {
			return { m_nodes.begin(), this };
		}
		iterator end() {
			return { m_nodes.end(), this };
		}

		const_iterator begin()const {
			return { m_nodes.begin(), this };
		}
		const_iterator end()const {
			return { m_nodes.end(), this };
		}

		template<typename Action>
		void query( rect_type const& region, Action&& action ) {
			query( region, m_nodes[ 0 ], std::forward<Action>( action ) );
		}

		iterator erase( const_iterator it ) {
			assert( it.m_iter->empty() && "Cannot remove node that is not empty" );
			return { m_nodes.erase( it.m_iter ), this };
		}
		void clear() {
			m_nodes.clear();
			m_count = 0;
		}

		auto size()const noexcept {
			return m_count;
		}

	private:
		auto find_containing_quadrant(
			rect_type const& parent_bounds,
			rect_type const& rect,
			std::size_t start_index,
			std::size_t end_index )const noexcept
			->std::optional<std::pair<std::size_t, rect_type>>
		{
			for( auto i = start_index; i < end_index; ++i ) {
				auto quadrant = get_quadrant( i - start_index, parent_bounds );
				if( !rect_traits::contains( quadrant, rect ) )continue;

				return { std::pair{ i, quadrant } };
			}

			return {};
		}
		auto find_intersecting_quadrant(
			rect_type const& parent_bounds,
			rect_type const& rect,
			std::size_t start_index,
			std::size_t end_index )const noexcept
			->std::optional<std::pair<std::size_t, rect_type>>
		{
			for( auto i = start_index; i < end_index; ++i ) {
				auto quadrant = get_quadrant( i - start_index, parent_bounds );
				if( !rect_traits::intersects( quadrant, rect ) )continue;

				return { std::pair{ i, quadrant } };
			}

			return {};
		}

		void sort( node_type* parent ) {
			const auto start_index = calc_child_start( parent->m_id );
			const auto end_index = calc_child_end( parent->m_id );
			const auto pid = parent->m_id;

			auto temp = std::move( parent->elements() );
			for( auto& obj : temp ) {
				auto rect = m_get_rect( obj );
				auto result =
					find_containing_quadrant( parent->bounds(), rect, start_index, end_index );

				if( !result.has_value() ) {
					parent = std::addressof( m_nodes[ pid ] );
					parent->elements().push_back( std::move( obj ) );
				}
				else {
					auto [i, quadrant] = *result;

					if( m_nodes.size() < end_index ) {
						m_nodes.resize( end_index );
						parent = std::addressof( m_nodes[ pid ] );
					}

					if( !m_nodes[ i ] )
						m_nodes[ i ] = { i, quadrant };

					add_data( std::addressof( m_nodes[ i ] ), rect, std::move( obj ) );
				}
			}
		}
		void add_data( rect_type const& obj_bounds, value_type const& data ) {
			auto* root = std::addressof( m_nodes.front() );
			add_data( root, obj_bounds, data );
		}
		void add_data( rect_type const& obj_bounds, value_type&& data ) {
			auto* root = std::addressof( m_nodes.front() );
			add_data( root, obj_bounds, std::move( data ) );
		}

		void add_data( node_type* parent, rect_type const& obj_bounds, value_type data ) {
			parent->elements().push_back( std::move( data ) );
			if( parent->count() > max_objects ) {
				sort( parent );
			}
		}

		template<typename Action>
		void query( rect_type const& region, node_type& parent, Action&& action ) {
			if( !rect_traits::intersects( region, parent.bounds() ) ) return;

			std::for_each( parent.elements().begin(), parent.elements().end(), action );

			const auto start_index = calc_child_start( parent.m_id );
			if( start_index >= m_nodes.size() )return;

			const auto end_index = calc_child_end( parent.m_id );
			if( end_index > m_nodes.size() )return;

			for( auto i = start_index; i < end_index; ++i ) {
				if( !m_nodes[ i ] )continue;
				if( !rect_traits::intersects( m_nodes[ i ].bounds(), region ) )continue;
				query( region, m_nodes[ i ], std::forward<Action>( action ) );
			}
		}

		std::size_t calc_child_start( std::size_t parent_index )const noexcept {
			return ( parent_index * std::size_t{ 4 } ) + std::size_t{ 1 };
		}
		std::size_t calc_child_end( std::size_t parent_index )const noexcept {
			return ( parent_index * std::size_t{ 4 } ) + std::size_t{ 5 };
		}
		std::size_t calc_parent_index( std::size_t child_index )const noexcept {
			if( child_index != std::size_t{} )
				return ( child_index - std::size_t{ 1 } ) / std::size_t{ 4 };

			return std::size_t{};
		}
		rect_type get_quadrant( std::size_t index, rect_type const& bounds )const noexcept {
			auto [left, top, right, bottom] = std::tuple{
				rect_traits::left( bounds ),
				rect_traits::top( bounds ),
				rect_traits::right( bounds ),
				rect_traits::bottom( bounds )
			};

			const auto [cx, cy] = std::pair{
				std::midpoint( left, right ),
				std::midpoint( top, bottom )
			};

			switch( index ) {
				case 0: return rect_traits::construct( left, top, cx, cy );
				case 1: return rect_traits::construct( cx, top, right, cy );
				case 2: return rect_traits::construct( left, cy, cx, bottom );
				case 3: return rect_traits::construct( cx, cy, right, bottom );
			}

			assert( index >= 0 && index < 4 && "Index out of range" );
			return rect_type{};
		};

	private:
		std::vector<node_type> m_nodes;
		std::function<rect_type( const value_type& )> m_get_rect;
		std::size_t m_count = std::size_t{};
};