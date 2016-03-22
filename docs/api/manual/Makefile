PROJECT:=uvisor-rtos-docs
WORD_TEMPLATE:=template/pandoc-template.docx
DOCUMENTS:=\
	Roadmap.md \
	README.md \
	UseCases.md \
	Technical.md
EXAMPLES:=example.docx example.pdf
TARGETS:=$(PROJECT).docx $(PROJECT).pdf

#
# Command Line Tools used
#
TOOL_PANDOC:=pandoc
TOOL_OOWRITER:=oowriter

#
# Derived Settings
#
PUBLISH_DIR:=../publish

#
# Various Tool Settings
#
SETTINGS_PANDOC:=-S --reference-docx $(WORD_TEMPLATE) -f markdown+multiline_tables --number-sections

#
# Build System
#
.PHONY: all example publish install_fedora clean

all: $(TARGETS)

example: $(EXAMPLES)

publish: $(TARGETS) $(EXAMPLES)
	cp $^  $(PUBLISH_DIR)
	git -C $(PUBLISH_DIR) add $^
	git -C $(PUBLISH_DIR) commit -a -m "released DOC & PDF documents"
	git -C $(PUBLISH_DIR) push

%.pdf : %.docx
	$(TOOL_OOWRITER) --headless --convert-to pdf $^

$(PROJECT).docx : $(DOCUMENTS)
	$(TOOL_PANDOC) $(SETTINGS_PANDOC) -o $@ $^

%.docx : %.md
	$(TOOL_PANDOC) $(SETTINGS_PANDOC) -o $@ $^

install_fedora:
	sudo dnf install pandoc texlive-latex-bin libreoffice-writer fedora-repos-rawhide
	sudo dnf --enablerepo=rawhide update pandoc

clean:
	rm -f $(EXAMPLES) $(TARGETS)
