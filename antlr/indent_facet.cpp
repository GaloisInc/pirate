#include "indent_facet.hpp"

/// I hate the way I solved this, but I can't think of a better way
/// around the problem.  I even asked stackoverflow for help:
///
///   http://stackoverflow.com/questions/32480237/apply-a-facet-to-all-stream-output-use-custom-string-manipulators
///
///
namespace indent_manip {

static const int index = std::ios_base::xalloc();

std::ostream& push(std::ostream& os) {
	auto ilevel = ++os.iword(index);
	os.imbue(std::locale(os.getloc(), new indent_facet(ilevel)));
	return os;
}

std::ostream& pop(std::ostream& os) {
	auto ilevel = (os.iword(index)>0) ? --os.iword(index) : 0;
	os.imbue(std::locale(os.getloc(), new indent_facet(ilevel)));
	return os;
}

/// Clears the ostream indentation set, but NOT the raii_guard.
std::ostream& clear(std::ostream& os) {
	os.iword(index) = 0;
	os.imbue(std::locale(os.getloc(), new indent_facet(0)));
	return os;
}

}
