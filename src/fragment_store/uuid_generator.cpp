#include "./uuid_generator.hpp"

UUIDGenerator_128_128::UUIDGenerator_128_128(void) {
	{ // random namespace
		const auto num0 = _rng();
		const auto num1 = _rng();
		const auto num2 = _rng();
		const auto num3 = _rng();

		_uuid_namespace[0+0] = (num0 >> 0) & 0xff;
		_uuid_namespace[0+1] = (num0 >> 8) & 0xff;
		_uuid_namespace[0+2] = (num0 >> 16) & 0xff;
		_uuid_namespace[0+3] = (num0 >> 24) & 0xff;

		_uuid_namespace[4+0] = (num1 >> 0) & 0xff;
		_uuid_namespace[4+1] = (num1 >> 8) & 0xff;
		_uuid_namespace[4+2] = (num1 >> 16) & 0xff;
		_uuid_namespace[4+3] = (num1 >> 24) & 0xff;

		_uuid_namespace[8+0] = (num2 >> 0) & 0xff;
		_uuid_namespace[8+1] = (num2 >> 8) & 0xff;
		_uuid_namespace[8+2] = (num2 >> 16) & 0xff;
		_uuid_namespace[8+3] = (num2 >> 24) & 0xff;

		_uuid_namespace[12+0] = (num3 >> 0) & 0xff;
		_uuid_namespace[12+1] = (num3 >> 8) & 0xff;
		_uuid_namespace[12+2] = (num3 >> 16) & 0xff;
		_uuid_namespace[12+3] = (num3 >> 24) & 0xff;
	}
}

UUIDGenerator_128_128::UUIDGenerator_128_128(const std::array<uint8_t, 16>& uuid_namespace) :
	_uuid_namespace(uuid_namespace)
{
}

std::vector<uint8_t> UUIDGenerator_128_128::operator()(void) {
	std::vector<uint8_t> new_uid(_uuid_namespace.cbegin(), _uuid_namespace.cend());
	new_uid.resize(new_uid.size() + 16);

	const auto num0 = _rng();
	const auto num1 = _rng();
	const auto num2 = _rng();
	const auto num3 = _rng();

	new_uid[_uuid_namespace.size()+0] = (num0 >> 0) & 0xff;
	new_uid[_uuid_namespace.size()+1] = (num0 >> 8) & 0xff;
	new_uid[_uuid_namespace.size()+2] = (num0 >> 16) & 0xff;
	new_uid[_uuid_namespace.size()+3] = (num0 >> 24) & 0xff;

	new_uid[_uuid_namespace.size()+4+0] = (num1 >> 0) & 0xff;
	new_uid[_uuid_namespace.size()+4+1] = (num1 >> 8) & 0xff;
	new_uid[_uuid_namespace.size()+4+2] = (num1 >> 16) & 0xff;
	new_uid[_uuid_namespace.size()+4+3] = (num1 >> 24) & 0xff;

	new_uid[_uuid_namespace.size()+8+0] = (num2 >> 0) & 0xff;
	new_uid[_uuid_namespace.size()+8+1] = (num2 >> 8) & 0xff;
	new_uid[_uuid_namespace.size()+8+2] = (num2 >> 16) & 0xff;
	new_uid[_uuid_namespace.size()+8+3] = (num2 >> 24) & 0xff;

	new_uid[_uuid_namespace.size()+12+0] = (num3 >> 0) & 0xff;
	new_uid[_uuid_namespace.size()+12+1] = (num3 >> 8) & 0xff;
	new_uid[_uuid_namespace.size()+12+2] = (num3 >> 16) & 0xff;
	new_uid[_uuid_namespace.size()+12+3] = (num3 >> 24) & 0xff;

	return new_uid;
}

