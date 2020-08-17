# quadtree

Using the Chili Framework for testing a C++ template quadtree implementation complete with iterators compatible with most of the standard algorithms that support bidirection iterators.

The quadtree and iterators can be found in qtree.h.  There are two versions of the qtree in different namespaces used for testing performance.  The one in the primary_qtree stores a vector of objects in the qtree class, and you call a commit() function to sort all of the objects into the nodes.  Each node stores a pointer to the objects in the qtree. 

The one in the value_qtree namespace uses values stored in each node.  A vector of all the created node pointers is stored to make creating iterators and iterating over all the objects a lot easier and frankly probably a lot faster.

Still a work in progress, so things will change.  The other things to note are in Rect.h, Vec2.h, rect_traits.h and vector_traits.h.

A quad tree needs boundaries and those boundaries are split up into quadrants.  Those quadrants are continually divided and each node is defined by those boundaries.  Assuming that you agree that an object representing these boundaries is a lot nicer to deal with, you'll need to have a struct/class for handling that.  Now, several 2D graphics libraries already supply their own rect classes as well as 2D vector classes, so I didn't want to create another one that required conversions between mine and the one you use for your graphics api.  So I created templated interfaces aptly named rect_traits<> and vector2_traits<>.  

You'll need to supply these traits classes with another traits class telling my traits classes how to access the members of the rectangle or vector2 classes you use.  

For the rectangle, you'll need to provide a struct with some static constexpr noexcept methods
```cpp
struct RectMemberAcess{
  using rect_type =   ;// whatever rectangle class you are using
  using scalar_type = ;// the data type of the members in the rectangle class

  static constexpr rect_type construct(scalar_type left_, scalar_type top_, scalar_type right scalar_type bottom )noexcept;
  static constexpr scalar_type left( rect_type const& )noexcept;
  static constexpr scalar_type top( rect_type const& )noexcept;
  static constexpr scalar_type right( rect_type const& )noexcept;
  static constexpr scalar_type width( rect_type const& )noexcept;
  static constexpr scalar_type height( rect_type const& )noexcept;
};
```
For the 2D vector, you'll need to provide a structure with some static constexpr noexcept methods:
```cpp
struct VectorMemberAccess{
  using vector_type = ;// whatever 2d vector you want to use
  using scalar_type = ;// the data type of x and y

  static constexpr construct( scalar_type, scalar_type )noexcept;
  static constexpr x( vector_type const& )noexcept;  // getter function
  static constexpr y( vector_type const& )noexcept;  // getter function
  static constexpr x( vector_type&, scalar_type )noexcept; // setter function
  static constexpr y( vector_type&, scalar_type )noexcept; // setter function
};```
The rect_traits<> class has more functionality such as rect_traits::intersects() and rect_traits::contains(), so you don't have to create your own.  The vector2_traits class has length(), length_sqr(), dot() and normalize() functions which can all be used just with the member access struct you provide.

Features implemented that work:
-Adding objects to nodes
-Sorting objects into the nodes
-Removing objects
-Querying the tree to return a vector of object pointers within a region

Features implemented that partially work:
-Iterators and const iterators

Features implemented, but not fully tested:
-Finding a specific object in the tree
-Removal of empty nodes when removing the last object from a node
