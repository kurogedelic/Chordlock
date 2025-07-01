/**
 * Spectrum Visualizer - Canvas-based frequency spectrum visualization
 */
class SpectrumVisualizer {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        
        // Configuration
        this.width = this.canvas.width;
        this.height = this.canvas.height;
        this.barWidth = 2;
        this.barSpacing = 1;
        this.maxBars = Math.floor(this.width / (this.barWidth + this.barSpacing));
        
        // Visual settings
        this.backgroundColor = '#f7fafc';
        this.barColor = '#667eea';
        this.peakColor = '#e53e3e';
        this.gridColor = '#e2e8f0';
        this.labelColor = '#4a5568';
        
        // Peak tracking
        this.peaks = new Array(this.maxBars).fill(0);
        this.peakDecay = 0.95;
        
        // Frequency range settings
        this.minFreq = 20;
        this.maxFreq = 20000;
        this.logScale = true;
        
        // Initialize
        this.setupCanvas();
        this.clear();
    }

    setupCanvas() {
        // Set up high DPI display support
        const dpr = window.devicePixelRatio || 1;
        const rect = this.canvas.getBoundingClientRect();
        
        this.canvas.width = rect.width * dpr;
        this.canvas.height = rect.height * dpr;
        this.canvas.style.width = rect.width + 'px';
        this.canvas.style.height = rect.height + 'px';
        
        this.ctx.scale(dpr, dpr);
        this.width = rect.width;
        this.height = rect.height;
    }

    clear() {
        this.ctx.fillStyle = this.backgroundColor;
        this.ctx.fillRect(0, 0, this.width, this.height);
        this.drawGrid();
    }

    drawGrid() {
        this.ctx.strokeStyle = this.gridColor;
        this.ctx.lineWidth = 1;
        
        // Horizontal grid lines (dB levels)
        const gridLines = 5;
        for (let i = 0; i <= gridLines; i++) {
            const y = (i / gridLines) * this.height;
            this.ctx.beginPath();
            this.ctx.moveTo(0, y);
            this.ctx.lineTo(this.width, y);
            this.ctx.stroke();
            
            // dB labels
            const db = Math.round(-90 + (i / gridLines) * 90);
            this.ctx.fillStyle = this.labelColor;
            this.ctx.font = '10px monospace';
            this.ctx.fillText(db + 'dB', 5, y - 2);
        }
        
        // Vertical grid lines (frequency markers)
        const freqMarkers = [100, 1000, 10000];
        freqMarkers.forEach(freq => {
            if (freq >= this.minFreq && freq <= this.maxFreq) {
                const x = this.frequencyToX(freq);
                this.ctx.beginPath();
                this.ctx.moveTo(x, 0);
                this.ctx.lineTo(x, this.height);
                this.ctx.stroke();
                
                // Frequency labels
                this.ctx.fillStyle = this.labelColor;
                this.ctx.font = '10px monospace';
                this.ctx.fillText(this.formatFrequency(freq), x + 2, this.height - 5);
            }
        });
    }

    frequencyToX(frequency) {
        if (this.logScale) {
            const logMin = Math.log10(this.minFreq);
            const logMax = Math.log10(this.maxFreq);
            const logFreq = Math.log10(frequency);
            return ((logFreq - logMin) / (logMax - logMin)) * this.width;
        } else {
            return ((frequency - this.minFreq) / (this.maxFreq - this.minFreq)) * this.width;
        }
    }

    formatFrequency(freq) {
        if (freq >= 1000) {
            return (freq / 1000).toFixed(1) + 'k';
        }
        return freq.toString();
    }

    update(frequencyData, sampleRate) {
        if (!frequencyData || frequencyData.length === 0) return;
        
        // Clear canvas
        this.clear();
        
        // Calculate frequency per bin
        const nyquist = sampleRate / 2;
        const binWidth = nyquist / frequencyData.length;
        
        // Draw spectrum bars
        const barsDrawn = Math.min(this.maxBars, frequencyData.length);
        
        for (let i = 0; i < barsDrawn; i++) {
            const frequency = i * binWidth;
            
            // Skip frequencies outside our range
            if (frequency < this.minFreq || frequency > this.maxFreq) continue;
            
            const magnitude = frequencyData[i];
            const normalizedMagnitude = magnitude / 255.0;
            
            // Convert to dB scale
            const dB = 20 * Math.log10(normalizedMagnitude + 1e-10);
            const normalizedDB = Math.max(0, (dB + 90) / 90); // -90dB to 0dB -> 0 to 1
            
            const x = this.frequencyToX(frequency);
            const barHeight = normalizedDB * this.height;
            const y = this.height - barHeight;
            
            // Update peak tracking
            const barIndex = Math.floor(i * this.maxBars / frequencyData.length);
            if (barIndex < this.peaks.length) {
                this.peaks[barIndex] = Math.max(this.peaks[barIndex] * this.peakDecay, normalizedDB);
            }
            
            // Draw bar with gradient
            const gradient = this.ctx.createLinearGradient(0, this.height, 0, 0);
            gradient.addColorStop(0, this.barColor);
            gradient.addColorStop(0.7, this.barColor + '80');
            gradient.addColorStop(1, this.barColor + '40');
            
            this.ctx.fillStyle = gradient;
            this.ctx.fillRect(x, y, this.barWidth, barHeight);
            
            // Draw peak marker
            if (this.peaks[barIndex] > 0.1) {
                const peakY = this.height - (this.peaks[barIndex] * this.height);
                this.ctx.fillStyle = this.peakColor;
                this.ctx.fillRect(x, peakY - 1, this.barWidth, 2);
            }
        }
        
        // Draw musical note frequency markers
        this.drawNoteMarkers();
    }

    drawNoteMarkers() {
        const musicalNotes = [
            { note: 'C2', freq: 65.4 },
            { note: 'C3', freq: 130.8 },
            { note: 'C4', freq: 261.6 },
            { note: 'C5', freq: 523.3 },
            { note: 'C6', freq: 1046.5 },
            { note: 'C7', freq: 2093.0 }
        ];
        
        this.ctx.strokeStyle = '#38a169';
        this.ctx.lineWidth = 1;
        this.ctx.setLineDash([2, 2]);
        
        musicalNotes.forEach(({ note, freq }) => {
            if (freq >= this.minFreq && freq <= this.maxFreq) {
                const x = this.frequencyToX(freq);
                
                this.ctx.beginPath();
                this.ctx.moveTo(x, 0);
                this.ctx.lineTo(x, this.height);
                this.ctx.stroke();
                
                // Note label
                this.ctx.fillStyle = '#38a169';
                this.ctx.font = '9px monospace';
                this.ctx.fillText(note, x + 2, 15);
            }
        });
        
        this.ctx.setLineDash([]);
    }

    updateSettings(settings) {
        if (settings.minFreq !== undefined) this.minFreq = settings.minFreq;
        if (settings.maxFreq !== undefined) this.maxFreq = settings.maxFreq;
        if (settings.logScale !== undefined) this.logScale = settings.logScale;
        if (settings.peakDecay !== undefined) this.peakDecay = settings.peakDecay;
    }

    // Highlight specific frequency ranges (useful for showing detected notes)
    highlightFrequencies(frequencies, color = '#e53e3e') {
        this.ctx.strokeStyle = color;
        this.ctx.lineWidth = 2;
        
        frequencies.forEach(freq => {
            if (freq >= this.minFreq && freq <= this.maxFreq) {
                const x = this.frequencyToX(freq);
                this.ctx.beginPath();
                this.ctx.moveTo(x, 0);
                this.ctx.lineTo(x, this.height);
                this.ctx.stroke();
            }
        });
    }

    // Export current visualization as image
    exportImage() {
        return this.canvas.toDataURL('image/png');
    }

    // Resize handler
    resize() {
        this.setupCanvas();
        this.clear();
    }
}

// Export for use in other modules
window.SpectrumVisualizer = SpectrumVisualizer;