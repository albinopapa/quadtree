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
};
```
The rect_traits<> class has more functionality such as rect_traits::intersects() and rect_traits::contains(), so you don't have to create your own.  The vector2_traits class has length(), length_sqr(), dot() and normalize() functions which can all be used just with the member access struct you provide.

Features implemented that work:
- Adding objects to nodes
- Sorting objects into the nodes
- Removing objects
- Querying the tree 
- Iterators and const iterators

Features no longer implemented
- Finding a specific object in the tree
- Removal of empty nodes when removing the last object from a node

NOTE: The way this version of the quadtree is implemented there is no longer a clear reason to include methods to find objects and somewhat delete empty nodes.  Instead of the traditional linked list of child nodes, the entire tree is a vector of nodes.  Iterating is done by starting at index 0 to end of vector.  As the tree grows, the vector is reallocated to be at least the size of the last child node to the current parent.  For example if the current parent index is 100, the child nodes would be 401, 402, 403 and 404 so the vector is resized to be 405 elements long.  While iterating, the operator++ and operator-- are designed to skip empty or uninitialized nodes so there is never a need to test if an iterator is valid other than checking against the iterator returned by qtree::end().  

Speaking of iterators, because of the nature of quadtrees each node is going to contain multiple objects so instead of iterating over each object, the iterators only iterate over the nodes and have accessor methods for getting the bounds of that node and a reference to the vector of objects contained within the node.  So implementing a find() function didn't make much sense to me.  You can iterate over the nodes and use std::find/std::find_if on the nodes' vector to find an object.

In other implementations I've seen around the hub or online the qtree::query() function would return a vector of pointers to objects within a region.  This implementation instead requires you pass a function object or functor to the query function that accepts an object of qtree::reference or qtree::const_reference.  In the sample code, I'm using Ball as the value_type type, so my functor requires a Ball& or Ball const& as it's paraemter.  Upon testing, this yielded much better results than returning a vector of pointers by almost 2x.

Removing objects from the tree can be done by finding the node the object is in, then erasing an element from the vector blonging to that node.  Do not try removing and reinserting in a single loop, this will cause issues as reinsertion can cause the tree to try to rebalance which could invalidate both the node iterator and the vector iterator contained within the node.  

Checking to see if objects are still within the node boundaries is the user's responsibility.  If an object is no longer within the node's boundary, it must be removed and reinserted.  This can be seen in the sample code.  I move the object into a temp vector and when the update loop is finished, I reinsert all the objects in the temp vector.

Other caveats.

I don't have a minimum node size test nor tree depth test.  The tree merely grows as long as an object can be contained within a node.  So if your object's size is particularly small, then depth could be high especially if your initial world size is quite large.  My suggestion for allowed_objects_per_node ( first template parameter of qtree ) is about 2% of the area of the root node's area.  In the sample code, the root node are or world size if 10,000x10,000 ( pixels in this case ).  Each ball is 20x20 ( radius = 10 ).  So maxing out the area of the tree would give me 250,000 balls.  The sample code has around 20,000 balls and an allowed_objects_per_node at 100.  This means that 100 balls would take up 200x200 ( 40,000 ) units of space which is about.  
