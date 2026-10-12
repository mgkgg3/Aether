#include "MeshParser.h"

#include <features/opone/OperationOne.h>

#include <cctype>
#include <cstring>

namespace mesh
{
	namespace parser
	{
		namespace
		{
			bool name_has(const std::string& s, const char* needle)
			{
				if (s.empty() || !needle)
					return false;
				std::string a = s;
				std::string b = needle;
				for (char& c : a) c = (char)std::tolower((unsigned char)c);
				for (char& c : b) c = (char)std::tolower((unsigned char)c);
				return a.find(b) != std::string::npos;
			}

			bool is_skip_part_name(const std::string& name)
			{
				if (name.empty())
					return false;
				if (name == "HumanoidRootPart" || name == "CollisionCapsule")
					return true;
				return name_has(name, "collision") || name_has(name, "hitbox") ||
				       name_has(name, "capsule") || name_has(name, "nocol");
			}

			bool is_body_name(const std::string& name)
			{
				static const char* const k_body[] = {

					"Head", "Torso", "UpperTorso", "LowerTorso",
					"LeftUpperArm", "LeftLowerArm", "LeftHand", "Left Arm",
					"RightUpperArm", "RightLowerArm", "RightHand", "Right Arm",
					"LeftUpperLeg", "LeftLowerLeg", "LeftFoot", "Left Leg",
					"RightUpperLeg", "RightLowerLeg", "RightFoot", "Right Leg",

					"head", "torso", "arm1", "arm2", "leg1", "leg2", "legs",
					"shoulder1", "shoulder2", "hip1", "hip2"
				};

				if (name.empty()) return false;
				for (const char* n : k_body)
				{
					if (name.size() != std::strlen(n)) continue;
					std::size_t i = 0;
					for (; i < name.size(); ++i)
					{
						const unsigned char a = (unsigned char)name[i];
						const unsigned char b = (unsigned char)n[i];
						if (std::tolower(a) != std::tolower(b)) break;
					}
					if (i == name.size()) return true;
				}
				return false;
			}

			kind classify(const std::string& name)
			{
				if (is_body_name(name))
					return kind::body;

				if (name_has(name, "hair") || name_has(name, "ponytail") ||
				    name_has(name, "bun") || name_has(name, "beetle") ||
				    name_has(name, "ringo"))
					return kind::hair;
				if (name_has(name, "face"))
					return kind::face;

				return kind::accessory;
			}
		}

		const char* kind_name(kind k)
		{
			switch (k)
			{
			case kind::body:      return "body";
			case kind::accessory: return "accessory";
			case kind::face:      return "face";
			case kind::hair:      return "hair";
			case kind::special:   return "special";
			default:              return "other";
			}
		}

		std::vector<entry_t> collect(
			const std::unordered_map<std::string, rbx::part_t>& parts)
		{
			std::vector<entry_t> out;
			out.reserve(parts.size());

			for (const auto& kv : parts)
			{
				if (!kv.second.address)
					continue;
				if (is_skip_part_name(kv.first))
					continue;

				if (op1::is_collider_body(kv.first) && op1::parts_contain_body(parts))
					continue;

				entry_t e{};
				e.part = kv.second.address;
				e.name = kv.first;
				e.what = classify(kv.first);
				out.push_back(std::move(e));
			}

			return out;
		}
	}
}
