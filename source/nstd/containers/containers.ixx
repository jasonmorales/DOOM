export module nstd.containers;

export namespace nstd {

template<typename T>
requires requires(T t) { std::rbegin(t); std::rend(t); }
class reverse_view
{
public:
	reverse_view(T& container) : container{container} {}

	auto begin() const { return container.rbegin(); }
	auto end() const { return container.rend(); }

	auto begin() { return container.rbegin(); }
	auto end() { return container.rend(); }

	T& container;
};

} // export namespace nstd
