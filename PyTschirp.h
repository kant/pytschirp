#pragma once

#include <pybind11/pybind11.h>

#include "SynthParameterDefinition.h"
#include "Synth.h"
#include "Patch.h"
#include "AutoDetection.h"

#include <algorithm>

namespace py = pybind11;


template<typename PATCH, typename ATTRIBUTE>
class PyTschirp {
public:
	PyTschirp() {
		patch_ = std::make_shared<PATCH>();
	}

	PyTschirp(std::shared_ptr<midikraft::Patch> p, std::weak_ptr<midikraft::Synth> synth) {
		// Downcast possible?
		auto correctPatch = std::dynamic_pointer_cast<PATCH>(p);
		if (!correctPatch) {
			throw std::runtime_error("PyTschirp: Can't downcast, wrong patch type! Program error");
		}
		patch_ = correctPatch;
		synth_ = synth;
	}

	ATTRIBUTE attr(std::string const &param) {
		return ATTRIBUTE(patch_, param);
	}

	ATTRIBUTE get_attr(std::string const &attrName) {
		try {
			return ATTRIBUTE(patch_, attrName);
		}
		catch (std::runtime_error &) {
			// That doesn't seem to exist... try with spaces instead of underscores in the name
			return ATTRIBUTE(patch_, underscoreToSpace(attrName));
		}
	}

	void set_attr(std::string const &name, int value) {
		try {
			auto attr = ATTRIBUTE(patch_, name);
			attr.setV(value);
			if (!synth_.expired() && synth_.lock()->channel().isValid()) {
				// The synth is hot... we don't know if this patch is currently selected, but let's send the nrpn or other value changing message anyway!
				auto messages = attr.def()->setValueMessage(*patch_, synth_.lock().get());
				midikraft::MidiController::instance()->getMidiOutput(synth_.lock()->midiOutput())->sendBlockOfMessagesNow(messages);
			}
		}
		catch (std::runtime_error &) {
			auto attr = ATTRIBUTE(patch_, underscoreToSpace(name));
			attr.setV(value);
		}
	}

	std::string layerName(int layerNo) {
		auto layeredPatch = std::dynamic_pointer_cast<midikraft::LayeredPatch>(patch_);
		if (layeredPatch) {
			if (layerNo >= 0 && layerNo < layeredPatch->numberOfLayers()) {
				return layeredPatch->layerName(layerNo);
			}
			throw std::runtime_error("PyTschirp: Invalid layer number given to layerName()");
		}
		if (layerNo == 0) {
			// Not a layered patch, but hey, layer 0 is good
			return patch_->patchName();
		}
		throw std::runtime_error("PyTschirp: This is not a layered patch, can't retrieve name of layer");
	}

private:
	std::string underscoreToSpace(std::string const &input) {
		auto copy = input;
		std::replace(copy.begin(), copy.end(), '_', ' ');
		return copy;
	}

	std::shared_ptr<PATCH> patch_;
	std::weak_ptr<midikraft::Synth> synth_;
};

