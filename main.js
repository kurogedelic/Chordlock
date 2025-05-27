createChordlockModule().then((Module) => {
	const startNote = 48; // C3
	const endNote = 72; // C5（非含む）
	const pressedNotes = new Set();
	const keyElements = {};
	const whiteNotes = [0, 2, 4, 5, 7, 9, 11];
	// ONコード検知の初期設定
	let onChordDetection = true;
	
	// ONコード検知のトグル処理
	const onChordToggle = document.getElementById('onChordToggle');
	onChordToggle.addEventListener('change', () => {
		onChordDetection = onChordToggle.checked;
		Module._setOnChordDetection(onChordDetection);
	});
	// 初期化
	Module._setOnChordDetection(onChordDetection);

	function remap(note) {
		if (note < startNote || note >= endNote) return startNote + (note % 12);
		return note;
	}

	function createKeyboardUI() {
		const keyboard = document.getElementById("keyboard");
		let whiteIndex = 0;
		const whiteKeyPositions = [];

		for (let i = startNote; i < endNote; i++) {
			const noteInOctave = i % 12;
			const isBlack = !whiteNotes.includes(noteInOctave);
			const key = document.createElement("div");

			key.className = isBlack ? "black-key" : "white-key";
			key.style.position = "absolute";
			key.style.height = isBlack ? "60px" : "100px";
			key.style.width = isBlack ? "24px" : "40px";
			key.style.border = "1px solid #666";
			key.style.boxSizing = "border-box";
			key.style.zIndex = isBlack ? 2 : 1;

			if (!isBlack) {
				const x = whiteIndex * 40;
				key.style.left = `${x}px`;
				whiteKeyPositions.push({ note: i, x });
				whiteIndex++;
			}
			keyboard.appendChild(key);
			keyElements[i] = key;
		}

		// Second pass: position black keys between correct white keys
		for (let i = startNote; i < endNote; i++) {
			const noteInOctave = i % 12;
			const isBlack = !whiteNotes.includes(noteInOctave);
			if (!isBlack) continue;

			const el = keyElements[i];

			// For black keys, define the surrounding white note positions
			// Black key always sits between two white keys: lower and upper
			let lower = null;
			let upper = null;
			for (let j = i - 1; j > startNote - 2; j--) {
				if (whiteNotes.includes(j % 12)) {
					lower = whiteKeyPositions.find((w) => w.note === j);
					if (lower) break;
				}
			}
			for (let j = i + 1; j < endNote + 2; j++) {
				if (whiteNotes.includes(j % 12)) {
					upper = whiteKeyPositions.find((w) => w.note === j);
					if (upper) break;
				}
			}
			if (lower && upper) {
				const x = (lower.x + upper.x) / 2 - 12 + 20;
				el.style.left = `${x}px`;
			}
		}
	}

	function updateKeyHighlight() {
		for (let i = startNote; i < endNote; i++) {
			const el = keyElements[i];
			if (!el) continue;
			const isPressed = pressedNotes.has(i);
			const isBlack = el.className === "black-key";
			el.style.background = isPressed
				? isBlack
					? "#4af"
					: "#acf"
				: isBlack
				? "black"
				: "white";
		}
	}

	navigator.requestMIDIAccess().then((midi) => {
		for (let input of midi.inputs.values()) {
			input.onmidimessage = (msg) => {
				const [status, note, vel] = msg.data;
				const now = performance.now();
				const viewNote = remap(note);

				if ((status & 0xf0) === 0x90 && vel > 0) {
					Module._noteOn(note, now); // 元のノートでコード推定
					pressedNotes.add(viewNote); // 表示はマッピング後の位置に
				} else if ((status & 0xf0) === 0x80 || vel === 0) {
					Module._noteOff(note);
					pressedNotes.delete(viewNote);
				}

				updateKeyHighlight();
			};
		}
	});

	setInterval(() => {
		const now = performance.now();

		// メイン候補
		const pConf = Module._malloc(4);
		const pStr = Module._detectWithConfidence(now, pConf);
		const name = Module.UTF8ToString(pStr);
		const conf = Module.getValue(pConf, "float");
		Module._free(pConf);

		// 代替候補を２件取得
		const altNamesPtr = Module._malloc(8); // char**[2]
		const altConfsPtr = Module._malloc(8); // float [2]
		const altCount = Module._detectAlternatives(
			now,
			altNamesPtr,
			altConfsPtr,
			2
		);
		const alt0 =
			altCount > 0
				? Module.UTF8ToString(Module.getValue(altNamesPtr, "i32"))
				: "";
		const alt1 =
			altCount > 1
				? Module.UTF8ToString(Module.getValue(altNamesPtr + 4, "i32"))
				: "";
		const a0c = altCount > 0 ? Module.getValue(altConfsPtr, "float") : 0;
		const a1c = altCount > 1 ? Module.getValue(altConfsPtr + 4, "float") : 0;
		Module._free(altNamesPtr);
		Module._free(altConfsPtr);

		// 単音はノートのみ
		const textMain =
			name === "—"
				? "waiting..."
				: conf > 1.1
				? name // 単音時は conf=1.0 固定 → 括弧付けない
				: `${name} (${conf.toFixed(2)})`;

		const textAlt =
			alt0 && alt0 !== name
				? ` / ${alt0} (${a0c.toFixed(2)})` +
				  (alt1 && alt1 !== name ? ` , ${alt1} (${a1c.toFixed(2)})` : "")
				: "";

		document.getElementById("chord").textContent = textMain + textAlt;
	}, 120);

	createKeyboardUI();
});
