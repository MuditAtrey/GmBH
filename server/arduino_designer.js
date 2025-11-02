// Arduino R4 Minima Designer - Visual Programming Interface
// (c) 2025

// Pin definitions for Arduino R4 Minima
const ARDUINO_PINS = {
    digital: [
        { pin: 'D0', type: ['DIGITAL', 'SERIAL_RX'], available: false }, // Reserved for ESP8266
        { pin: 'D1', type: ['DIGITAL', 'SERIAL_TX'], available: false }, // Reserved for ESP8266
        { pin: 'D2', type: ['DIGITAL', 'PWM'], available: true },
        { pin: 'D3', type: ['DIGITAL', 'PWM'], available: true },
        { pin: 'D4', type: ['DIGITAL'], available: true },
        { pin: 'D5', type: ['DIGITAL', 'PWM'], available: true },
        { pin: 'D6', type: ['DIGITAL', 'PWM'], available: true },
        { pin: 'D7', type: ['DIGITAL'], available: true },
        { pin: 'D8', type: ['DIGITAL'], available: true },
        { pin: 'D9', type: ['DIGITAL', 'PWM'], available: true },
        { pin: 'D10', type: ['DIGITAL', 'PWM', 'SPI_CS'], available: true },
        { pin: 'D11', type: ['DIGITAL', 'PWM', 'SPI_MOSI'], available: true },
        { pin: 'D12', type: ['DIGITAL', 'SPI_MISO'], available: true },
        { pin: 'D13', type: ['DIGITAL', 'SPI_SCK'], available: true },
    ],
    analog: [
        { pin: 'A0', type: ['ANALOG'], available: true },
        { pin: 'A1', type: ['ANALOG'], available: true },
        { pin: 'A2', type: ['ANALOG'], available: true },
        { pin: 'A3', type: ['ANALOG'], available: true },
        { pin: 'A4', type: ['ANALOG', 'I2C_SDA'], available: true },
        { pin: 'A5', type: ['ANALOG', 'I2C_SCL'], available: true },
    ],
    power: [
        { pin: '5V', type: ['POWER'] },
        { pin: '3.3V', type: ['POWER'] },
        { pin: 'GND', type: ['GROUND'] },
    ]
};

// Device database with pin requirements
const DEVICE_DATABASE = {
    pca9685: {
        name: 'PCA9685 16-Channel PWM',
        category: 'Motor Controller',
        pinRequirements: {
            'I2C_SDA': 1,
            'I2C_SCL': 1,
            'POWER': 1,
            'GROUND': 1
        },
        sharable: ['I2C_SDA', 'I2C_SCL'], // I2C can be shared
        properties: {
            i2cAddress: { type: 'text', default: '0x40', label: 'I2C Address' },
            frequency: { type: 'number', default: 50, label: 'PWM Frequency (Hz)' }
        },
        canHaveChildren: true,
        childTypes: ['mg90s', 'dc_motor']
    },
    mg90s: {
        name: 'MG90S Servo Motor',
        category: 'Servo',
        pinRequirements: {
            'PWM': 1,
            'POWER': 1,
            'GROUND': 1
        },
        properties: {
            minAngle: { type: 'number', default: 0, label: 'Min Angle (°)' },
            maxAngle: { type: 'number', default: 180, label: 'Max Angle (°)' },
            defaultAngle: { type: 'number', default: 90, label: 'Default Angle (°)' }
        },
        hardware: {
            recommendations: 'Use PWM pins (D3, D5, D6, D9, D10, D11) for servo control',
            capacitor: '100µF - 470µF electrolytic capacitor across power rails (prevents voltage spikes)',
            wiring: 'Orange/Yellow → PWM Pin, Red → 5V, Brown/Black → GND',
            voltage: '4.8V - 6V (5V Arduino is perfect)',
            current: '~100-300mA under load, up to 650mA stall',
            notes: 'Multiple servos need external power supply! Arduino 5V pin limited to ~500mA total. Use large capacitor to smooth power.'
        }
    },
    motion: {
        name: 'PIR Motion Sensor',
        category: 'Sensor',
        pinRequirements: {
            'DIGITAL': 1,
            'POWER': 1,
            'GROUND': 1
        },
        properties: {
            sensitivity: { type: 'range', min: 0, max: 100, default: 50, label: 'Sensitivity %' }
        }
    },
    potentiometer: {
        name: '5-Pin Potentiometer',
        category: 'Input',
        pinRequirements: {
            'ANALOG': 1,
            'POWER': 1,
            'GROUND': 1
        },
        properties: {
            smoothing: { type: 'number', default: 10, label: 'Smoothing Samples' }
        }
    },
    dht22: {
        name: 'DHT22 Temp & Humidity',
        category: 'Sensor',
        pinRequirements: {
            'DIGITAL': 1,
            'POWER': 1,
            'GROUND': 1
        },
        properties: {
            pollInterval: { type: 'number', default: 2000, label: 'Poll Interval (ms)' }
        }
    },
    '9dof': {
        name: '9DOF IMU Sensor',
        category: 'Sensor',
        pinRequirements: {
            'I2C_SDA': 1,
            'I2C_SCL': 1,
            'POWER': 1,
            'GROUND': 1
        },
        sharable: ['I2C_SDA', 'I2C_SCL'],
        properties: {
            i2cAddress: { type: 'text', default: '0x68', label: 'I2C Address' }
        }
    },
    spi_oled: {
        name: 'SPI OLED Display (7-pin)',
        category: 'Display',
        pinRequirements: {
            'SPI_MOSI': 1,
            'SPI_SCK': 1,
            'SPI_CS': 1,
            'DIGITAL': 2, // DC and RST
            'POWER': 1,
            'GROUND': 1
        },
        sharable: ['SPI_MOSI', 'SPI_SCK'], // SPI bus can be shared
        properties: {
            width: { type: 'number', default: 128, label: 'Width (px)' },
            height: { type: 'number', default: 64, label: 'Height (px)' },
            defaultText: { type: 'text', default: 'Hello!', label: 'Default Text' }
        }
    },
    button: {
        name: 'Push Button',
        category: 'Input',
        pinRequirements: {
            'DIGITAL': 1,
            'GROUND': 1
        },
        properties: {
            pullup: { type: 'checkbox', default: true, label: 'Use Internal Pullup' },
            debounce: { type: 'number', default: 50, label: 'Debounce (ms)' }
        },
        hardware: {
            recommendations: 'Any digital pin (D2-D13) works well',
            resistor: '10kΩ pull-down resistor if not using internal pullup',
            wiring: 'With pullup: One side to pin, other to GND. Without: One to 5V, other to pin + 10kΩ resistor to GND',
            voltage: '5V tolerant',
            current: '~0.1mA (negligible)',
            notes: 'Internal pullup is recommended - cleaner circuit! Button press reads LOW with pullup, HIGH without.'
        }
    },
    led: {
        name: 'Simple LED',
        category: 'Output',
        pinRequirements: {
            'DIGITAL': 1,
            'GROUND': 1
        },
        properties: {
            brightness: { type: 'range', min: 0, max: 255, default: 255, label: 'Brightness' },
            blinkInterval: { type: 'number', default: 1000, label: 'Blink Interval (ms)' }
        },
        hardware: {
            recommendations: 'Use PWM pins (D3, D5, D6, D9, D10, D11) for brightness control',
            resistor: '220Ω - 330Ω current limiting resistor (prevents LED burnout)',
            wiring: 'Arduino Pin → Resistor → LED Anode (+) → LED Cathode (-) → GND',
            voltage: '5V from Arduino is safe with proper resistor',
            current: '~10-20mA typical draw',
            notes: 'Longer leg is anode (+), shorter is cathode (-). Never connect directly without resistor!'
        }
    },
    photoresistor: {
        name: 'Photo Resistor (LDR)',
        category: 'Sensor',
        pinRequirements: {
            'ANALOG': 1,
            'POWER': 1,
            'GROUND': 1
        },
        properties: {
            threshold: { type: 'number', default: 500, label: 'Light Threshold' }
        }
    },
    photodiode: {
        name: 'Photo Diode',
        category: 'Sensor',
        pinRequirements: {
            'ANALOG': 1,
            'POWER': 1,
            'GROUND': 1
        },
        properties: {
            sensitivity: { type: 'range', min: 0, max: 100, default: 50, label: 'Sensitivity %' }
        }
    }
};

// Global state
let currentConfig = {
    devices: [],
    pinAssignments: {},
    visualProgram: []
};

// Initialize the interface
function init() {
    renderPins();
    setupDragAndDrop();
    loadSavedConfig();
}

// Render pin visualization
function renderPins() {
    const digitalContainer = document.getElementById('digital-pins');
    const analogContainer = document.getElementById('analog-pins');
    const commContainer = document.getElementById('comm-pins');

    // Render digital pins
    ARDUINO_PINS.digital.forEach(pinData => {
        const pinEl = createPinElement(pinData);
        digitalContainer.appendChild(pinEl);
    });

    // Render analog pins
    ARDUINO_PINS.analog.forEach(pinData => {
        const pinEl = createPinElement(pinData);
        if (pinData.type.includes('I2C_SDA') || pinData.type.includes('I2C_SCL')) {
            commContainer.appendChild(pinEl);
        } else {
            analogContainer.appendChild(pinEl);
        }
    });
}

// Create pin element
function createPinElement(pinData) {
    const div = document.createElement('div');
    div.className = 'pin';
    div.dataset.pin = pinData.pin;
    
    if (!pinData.available) {
        div.classList.add('assigned');
    }
    
    // Add class based on pin type
    if (pinData.type.includes('I2C_SDA') || pinData.type.includes('I2C_SCL')) {
        div.classList.add('i2c');
    } else if (pinData.type.includes('SPI')) {
        div.classList.add('spi');
    } else if (pinData.type.includes('ANALOG')) {
        div.classList.add('analog');
    } else if (pinData.type.includes('PWM')) {
        div.classList.add('pwm');
    }
    
    div.innerHTML = `
        <div>
            <div class="pin-label">${pinData.pin}</div>
            <div class="pin-type">${pinData.type.join(', ')}</div>
            <div class="pin-device" id="pin-device-${pinData.pin}"></div>
        </div>
    `;
    
    div.addEventListener('click', () => selectPin(pinData.pin));
    
    return div;
}

// Setup drag and drop
function setupDragAndDrop() {
    const deviceLibrary = document.querySelector('.device-library');
    const configPanel = document.getElementById('config-devices');
    const programArea = document.querySelector('.program-area');
    
    // Touch support variables
    let touchDragData = {
        type: null,
        deviceType: null,
        blockType: null,
        blockClass: null,
        sourceElement: null
    };
    
    // Use event delegation for drag start (works even when items are updated)
    deviceLibrary.addEventListener('dragstart', (e) => {
        if (e.target.classList.contains('device-item')) {
            const deviceType = e.target.dataset.device;
            e.dataTransfer.effectAllowed = 'copy';
            e.dataTransfer.setData('deviceType', deviceType);
            e.dataTransfer.setData('dragType', 'device');
            e.target.style.opacity = '0.5';
            console.log('Drag started:', deviceType);
        }
    });
    
    deviceLibrary.addEventListener('dragend', (e) => {
        if (e.target.classList.contains('device-item')) {
            e.target.style.opacity = '1';
        }
    });
    
    // Touch support for device library
    deviceLibrary.addEventListener('touchstart', (e) => {
        if (e.target.classList.contains('device-item')) {
            const deviceType = e.target.dataset.device;
            touchDragData.type = 'device';
            touchDragData.deviceType = deviceType;
            touchDragData.sourceElement = e.target;
            e.target.style.opacity = '0.5';
            console.log('Touch drag started:', deviceType);
        }
    }, { passive: true });
    
    deviceLibrary.addEventListener('touchend', (e) => {
        if (touchDragData.sourceElement) {
            touchDragData.sourceElement.style.opacity = '1';
            
            // Get touch position
            const touch = e.changedTouches[0];
            const dropTarget = document.elementFromPoint(touch.clientX, touch.clientY);
            
            // Check if dropped on config panel
            if (dropTarget && (dropTarget.id === 'config-devices' || dropTarget.closest('#config-devices'))) {
                configPanel.style.background = '';
                if (touchDragData.deviceType) {
                    addDeviceToConfig(touchDragData.deviceType);
                    console.log('Touch dropped device:', touchDragData.deviceType);
                }
            }
            
            // Reset touch data
            touchDragData = { type: null, deviceType: null, blockType: null, blockClass: null, sourceElement: null };
        }
    }, { passive: true });
    
    // Config panel drop zone
    configPanel.addEventListener('dragover', (e) => {
        e.preventDefault();
        e.dataTransfer.dropEffect = 'copy';
        configPanel.style.background = 'rgba(102, 126, 234, 0.1)';
    });
    
    configPanel.addEventListener('dragleave', (e) => {
        configPanel.style.background = '';
    });
    
    configPanel.addEventListener('drop', (e) => {
        e.preventDefault();
        configPanel.style.background = '';
        const deviceType = e.dataTransfer.getData('deviceType');
        console.log('Dropped device:', deviceType);
        if (deviceType) {
            addDeviceToConfig(deviceType);
        }
    });
    
    // Touch move feedback for config panel
    configPanel.addEventListener('touchmove', (e) => {
        if (touchDragData.type === 'device') {
            configPanel.style.background = 'rgba(102, 126, 234, 0.1)';
        }
    }, { passive: true });
    
    configPanel.addEventListener('touchend', () => {
        configPanel.style.background = '';
    }, { passive: true });
    
    // Visual programming blocks drag and drop
    const blocks = document.querySelectorAll('.block');
    blocks.forEach(block => {
        block.addEventListener('dragstart', (e) => {
            e.dataTransfer.effectAllowed = 'copy';
            e.dataTransfer.setData('blockType', e.target.textContent);
            e.dataTransfer.setData('blockClass', e.target.className);
            e.dataTransfer.setData('dragType', 'block');
            e.target.style.opacity = '0.5';
        });
        
        block.addEventListener('dragend', (e) => {
            e.target.style.opacity = '1';
        });
        
        // Touch support for blocks
        block.addEventListener('touchstart', (e) => {
            touchDragData.type = 'block';
            touchDragData.blockType = e.target.textContent;
            touchDragData.blockClass = e.target.className;
            touchDragData.sourceElement = e.target;
            e.target.style.opacity = '0.5';
            console.log('Touch drag block started:', touchDragData.blockType);
        }, { passive: true });
        
        block.addEventListener('touchend', (e) => {
            if (touchDragData.sourceElement) {
                touchDragData.sourceElement.style.opacity = '1';
                
                // Get touch position
                const touch = e.changedTouches[0];
                const dropTarget = document.elementFromPoint(touch.clientX, touch.clientY);
                
                // Check if dropped on program area
                if (dropTarget && (dropTarget.classList.contains('program-area') || dropTarget.closest('.program-area'))) {
                    if (programArea) {
                        programArea.style.borderColor = '#cbd5e0';
                        programArea.style.background = '#edf2f7';
                    }
                    if (touchDragData.blockType) {
                        addBlockToProgram(touchDragData.blockType, touchDragData.blockClass);
                        console.log('Touch dropped block:', touchDragData.blockType);
                    }
                }
                
                // Reset touch data
                touchDragData = { type: null, deviceType: null, blockType: null, blockClass: null, sourceElement: null };
            }
        }, { passive: true });
    });
    
    // Program area drop zone
    if (programArea) {
        programArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            const dragType = e.dataTransfer.types.includes('dragtype') ? 'block' : 'unknown';
            if (dragType === 'block' || e.dataTransfer.types.includes('blocktype')) {
                e.dataTransfer.dropEffect = 'copy';
                programArea.style.borderColor = '#667eea';
                programArea.style.background = 'rgba(102, 126, 234, 0.05)';
            }
        });
        
        programArea.addEventListener('dragleave', (e) => {
            programArea.style.borderColor = '#cbd5e0';
            programArea.style.background = '#edf2f7';
        });
        
        programArea.addEventListener('drop', (e) => {
            e.preventDefault();
            programArea.style.borderColor = '#cbd5e0';
            programArea.style.background = '#edf2f7';
            
            const blockType = e.dataTransfer.getData('blockType');
            const blockClass = e.dataTransfer.getData('blockClass');
            
            if (blockType) {
                addBlockToProgram(blockType, blockClass);
            }
        });
        
        // Touch move feedback for program area
        programArea.addEventListener('touchmove', (e) => {
            if (touchDragData.type === 'block') {
                programArea.style.borderColor = '#667eea';
                programArea.style.background = 'rgba(102, 126, 234, 0.05)';
            }
        }, { passive: true });
        
        programArea.addEventListener('touchend', () => {
            programArea.style.borderColor = '#cbd5e0';
            programArea.style.background = '#edf2f7';
        }, { passive: true });
    }
}

// Add block to visual program
function addBlockToProgram(blockType, blockClass) {
    const programArea = document.querySelector('.program-area');
    
    // Create a new block instance
    const newBlock = document.createElement('div');
    newBlock.className = blockClass.replace('block', 'program-block');
    newBlock.innerHTML = `
        <span>${blockType}</span>
        <button class="remove-block-btn" onclick="this.parentElement.remove()">×</button>
    `;
    
    // Remove placeholder text if exists
    if (programArea.textContent.includes('Drop programming blocks here')) {
        programArea.innerHTML = '';
    }
    
    programArea.appendChild(newBlock);
    
    // Save to config
    if (!currentConfig.visualProgram) {
        currentConfig.visualProgram = [];
    }
    currentConfig.visualProgram.push({
        type: blockType,
        class: blockClass
    });
    
    console.log('Block added:', blockType);
}

// Add device to configuration
function addDeviceToConfig(deviceType, parentId = null) {
    const deviceInfo = DEVICE_DATABASE[deviceType];
    if (!deviceInfo) return;
    
    // Check if we have enough available pins
    if (!canAddDevice(deviceType)) {
        alert(`Cannot add ${deviceInfo.name}: Not enough available pins!`);
        return;
    }
    
    const deviceId = `${deviceType}_${Date.now()}`;
    const device = {
        id: deviceId,
        type: deviceType,
        name: deviceInfo.name,
        properties: {},
        assignedPins: {},
        children: []
    };
    
    // Set default properties
    for (const [prop, config] of Object.entries(deviceInfo.properties || {})) {
        device.properties[prop] = config.default;
    }
    
    if (parentId) {
        // Add as child device
        const parent = findDevice(parentId);
        if (parent) {
            parent.children.push(device);
        }
    } else {
        currentConfig.devices.push(device);
    }
    
    // Auto-assign pins
    autoAssignPins(device);
    
    renderConfig();
    updateAvailableDevices();
    updateStats();
}

// Check if device can be added
function canAddDevice(deviceType) {
    const deviceInfo = DEVICE_DATABASE[deviceType];
    const requirements = deviceInfo.pinRequirements;
    
    for (const [pinType, count] of Object.entries(requirements)) {
        if (pinType === 'POWER' || pinType === 'GROUND') continue;
        
        const availablePins = getAvailablePins(pinType);
        const sharablePins = deviceInfo.sharable?.includes(pinType) ? 
            getPinsByType(pinType).length : 0;
        
        if (availablePins.length + sharablePins < count) {
            return false;
        }
    }
    
    return true;
}

// Get available pins of specific type
function getAvailablePins(pinType) {
    const allPins = [...ARDUINO_PINS.digital, ...ARDUINO_PINS.analog];
    return allPins.filter(pin => 
        pin.available && 
        pin.type.some(t => t === pinType || (pinType === 'DIGITAL' && !t.includes('I2C') && !t.includes('SPI')))
    );
}

// Get pins by type (including assigned ones)
function getPinsByType(pinType) {
    const allPins = [...ARDUINO_PINS.digital, ...ARDUINO_PINS.analog];
    return allPins.filter(pin => pin.type.includes(pinType));
}

// Auto-assign pins to device
function autoAssignPins(device) {
    const deviceInfo = DEVICE_DATABASE[device.type];
    const requirements = deviceInfo.pinRequirements;
    
    for (const [pinType, count] of Object.entries(requirements)) {
        if (pinType === 'POWER' || pinType === 'GROUND') continue;
        
        // Check if pin type is sharable
        const canShare = deviceInfo.sharable?.includes(pinType);
        
        for (let i = 0; i < count; i++) {
            let assignedPin;
            
            if (canShare) {
                // Try to find already assigned sharable pin first
                const sharedPin = findSharedPin(pinType);
                if (sharedPin) {
                    assignedPin = sharedPin;
                } else {
                    // Assign new pin
                    const availablePins = getAvailablePins(pinType);
                    if (availablePins.length > 0) {
                        assignedPin = availablePins[0];
                        availablePins[0].available = false;
                    }
                }
            } else {
                // Find and assign exclusive pin
                const availablePins = getAvailablePins(pinType);
                if (availablePins.length > 0) {
                    assignedPin = availablePins[0];
                    availablePins[0].available = false;
                }
            }
            
            if (assignedPin) {
                const key = i === 0 ? pinType : `${pinType}_${i + 1}`;
                device.assignedPins[key] = assignedPin.pin;
                currentConfig.pinAssignments[assignedPin.pin] = device.id;
            }
        }
    }
}

// Find shared pin
function findSharedPin(pinType) {
    for (const [pin, deviceId] of Object.entries(currentConfig.pinAssignments)) {
        const device = findDevice(deviceId);
        if (device) {
            const deviceInfo = DEVICE_DATABASE[device.type];
            if (deviceInfo.sharable?.includes(pinType)) {
                const allPins = [...ARDUINO_PINS.digital, ...ARDUINO_PINS.analog];
                return allPins.find(p => p.pin === pin && p.type.includes(pinType));
            }
        }
    }
    return null;
}

// Find device by ID
function findDevice(id) {
    for (const device of currentConfig.devices) {
        if (device.id === id) return device;
        for (const child of device.children || []) {
            if (child.id === id) return child;
        }
    }
    return null;
}

// Render configuration panel
function renderConfig() {
    const container = document.getElementById('config-devices');
    container.innerHTML = '';
    
    if (currentConfig.devices.length === 0) {
        container.innerHTML = `
            <p style="color: #a0aec0; font-size: 13px; text-align: center; margin-top: 20px;">
                Drag devices from the library to add them to your config
            </p>
        `;
        return;
    }
    
    currentConfig.devices.forEach(device => {
        container.appendChild(createDeviceElement(device));
    });
    
    // Update pin visualization
    updatePinVisualization();
}

// Create device configuration element
function createDeviceElement(device) {
    const div = document.createElement('div');
    div.className = 'config-device';
    div.dataset.deviceId = device.id;
    
    const deviceInfo = DEVICE_DATABASE[device.type];
    
    let html = `
        <div class="header">
            <div class="device-name">${device.name}</div>
            <button class="remove-btn" onclick="removeDevice('${device.id}')">Remove</button>
        </div>
        <div style="font-size: 11px; color: #718096; margin-bottom: 10px;">
            Pins: ${Object.values(device.assignedPins).join(', ')}
        </div>
    `;
    
    // Hardware suggestions
    if (deviceInfo.hardware) {
        html += `
            <div class="hardware-suggestions">
                <div class="hardware-title" onclick="this.parentElement.classList.toggle('expanded')">
                    💡 Hardware Guide <span class="toggle-icon">▼</span>
                </div>
                <div class="hardware-content">
                    ${deviceInfo.hardware.recommendations ? `<div class="hw-item"><strong>📍 Pin Recommendations:</strong> ${deviceInfo.hardware.recommendations}</div>` : ''}
                    ${deviceInfo.hardware.resistor ? `<div class="hw-item"><strong>⚡ Resistor:</strong> ${deviceInfo.hardware.resistor}</div>` : ''}
                    ${deviceInfo.hardware.capacitor ? `<div class="hw-item"><strong>🔋 Capacitor:</strong> ${deviceInfo.hardware.capacitor}</div>` : ''}
                    ${deviceInfo.hardware.wiring ? `<div class="hw-item"><strong>🔌 Wiring:</strong> ${deviceInfo.hardware.wiring}</div>` : ''}
                    ${deviceInfo.hardware.voltage ? `<div class="hw-item"><strong>⚡ Voltage:</strong> ${deviceInfo.hardware.voltage}</div>` : ''}
                    ${deviceInfo.hardware.current ? `<div class="hw-item"><strong>🔌 Current Draw:</strong> ${deviceInfo.hardware.current}</div>` : ''}
                    ${deviceInfo.hardware.notes ? `<div class="hw-item hw-notes"><strong>⚠️ Important:</strong> ${deviceInfo.hardware.notes}</div>` : ''}
                </div>
            </div>
        `;
    }
    
    html += `<div class="properties">`;
    
    // Render properties
    for (const [prop, config] of Object.entries(deviceInfo.properties || {})) {
        html += createPropertyInput(device.id, prop, config, device.properties[prop]);
    }
    
    html += `</div>`;
    
    // Render children (for PCA9685 etc)
    if (deviceInfo.canHaveChildren) {
        html += `
            <div class="sub-devices">
                <div style="font-size: 12px; font-weight: 600; margin-bottom: 8px;">
                    Attached Devices (${device.children.length}/16)
                </div>
        `;
        
        device.children.forEach(child => {
            html += `
                <div class="sub-device">
                    ${child.name} - Channel ${device.children.indexOf(child)}
                    <button class="remove-btn" style="float: right;" onclick="removeDevice('${child.id}')">×</button>
                </div>
            `;
        });
        
        if (device.children.length < 16) {
            html += `<button class="add-sub-device" onclick="addSubDevice('${device.id}')">+ Add Servo/Motor</button>`;
        }
        
        html += `</div>`;
    }
    
    div.innerHTML = html;
    return div;
}

// Create property input
function createPropertyInput(deviceId, propName, config, value) {
    let input = '';
    
    switch (config.type) {
        case 'text':
            input = `<input type="text" value="${value}" onchange="updateProperty('${deviceId}', '${propName}', this.value)">`;
            break;
        case 'number':
            input = `<input type="number" value="${value}" onchange="updateProperty('${deviceId}', '${propName}', this.value)">`;
            break;
        case 'checkbox':
            input = `<input type="checkbox" ${value ? 'checked' : ''} onchange="updateProperty('${deviceId}', '${propName}', this.checked)">`;
            break;
        case 'select':
            input = `<select onchange="updateProperty('${deviceId}', '${propName}', this.value)">`;
            config.options.forEach(opt => {
                input += `<option value="${opt}" ${opt === value ? 'selected' : ''}>${opt}</option>`;
            });
            input += `</select>`;
            break;
        case 'range':
            input = `<input type="range" min="${config.min}" max="${config.max}" value="${value}" onchange="updateProperty('${deviceId}', '${propName}', this.value)">
                     <span>${value}</span>`;
            break;
    }
    
    return `
        <div class="property">
            <label>${config.label}</label>
            ${input}
        </div>
    `;
}

// Update property
function updateProperty(deviceId, propName, value) {
    const device = findDevice(deviceId);
    if (device) {
        device.properties[propName] = value;
        console.log(`Updated ${device.name} - ${propName}: ${value}`);
    }
}

// Remove device
function removeDevice(deviceId) {
    const device = findDevice(deviceId);
    if (!device) return;
    
    // Free up pins
    for (const pin of Object.values(device.assignedPins)) {
        delete currentConfig.pinAssignments[pin];
        const allPins = [...ARDUINO_PINS.digital, ...ARDUINO_PINS.analog];
        const pinData = allPins.find(p => p.pin === pin);
        if (pinData) pinData.available = true;
    }
    
    // Remove from config
    currentConfig.devices = currentConfig.devices.filter(d => d.id !== deviceId);
    
    // Remove from parent's children
    currentConfig.devices.forEach(d => {
        if (d.children) {
            d.children = d.children.filter(c => c.id !== deviceId);
        }
    });
    
    renderConfig();
    updateAvailableDevices();
    updateStats();
}

// Add sub-device (for PCA9685)
function addSubDevice(parentId) {
    addDeviceToConfig('mg90s', parentId);
}

// Update pin visualization
function updatePinVisualization() {
    // Clear all pin assignments
    document.querySelectorAll('.pin-device').forEach(el => el.textContent = '');
    
    // Update with current assignments
    for (const [pin, deviceId] of Object.entries(currentConfig.pinAssignments)) {
        const device = findDevice(deviceId);
        if (device) {
            const pinEl = document.getElementById(`pin-device-${pin}`);
            if (pinEl) {
                pinEl.textContent = device.name;
            }
            
            const pinDiv = document.querySelector(`[data-pin="${pin}"]`);
            if (pinDiv) {
                pinDiv.classList.add('assigned');
            }
        }
    }
}

// Update available devices based on pins
function updateAvailableDevices() {
    document.querySelectorAll('.device-item').forEach(item => {
        const deviceType = item.dataset.device;
        const canAdd = canAddDevice(deviceType);
        
        if (canAdd) {
            item.classList.remove('disabled');
        } else {
            item.classList.add('disabled');
        }
    });
}

// Update statistics
function updateStats() {
    const deviceCount = currentConfig.devices.length;
    const pinCount = Object.keys(currentConfig.pinAssignments).length;
    
    document.getElementById('device-count').textContent = `${deviceCount} devices configured`;
    document.getElementById('pin-usage').textContent = `${pinCount}/20 pins used`;
}

// Save configuration
function saveConfig() {
    const config = JSON.stringify(currentConfig, null, 2);
    console.log('Saving config:', config);
    
    // Send to server
    fetch('/api/config/save', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: config
    })
    .then(res => res.json())
    .then(data => {
        alert('✅ Configuration saved!');
        console.log('Saved:', data);
    })
    .catch(err => {
        alert('❌ Failed to save configuration');
        console.error(err);
    });
}

// Deploy to Arduino
function deployToArduino() {
    if (currentConfig.devices.length === 0) {
        alert('❌ No devices configured!');
        return;
    }
    
    console.log('Deploying to Arduino:', currentConfig);
    
    // Create a copy of config with built-in LED for testing
    const deployConfig = JSON.parse(JSON.stringify(currentConfig));
    
    // Add built-in LED (LED_BUILTIN on pin 13) for testing
    const hasBuiltinLED = deployConfig.devices.some(d => d.id === 'builtin_led');
    if (!hasBuiltinLED) {
        deployConfig.devices.push({
            id: 'builtin_led',
            type: 'LED',
            pin: 'D13',
            properties: {
                blinkInterval: 1000  // Blink every second for visual feedback
            }
        });
        console.log('✨ Added built-in LED for testing');
    }
    
    // Generate Arduino code
    const arduinoCode = generateArduinoCode();
    
    // Send to server for deployment
    fetch('/api/deploy', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ config: deployConfig, code: arduinoCode })
    })
    .then(res => res.json())
    .then(data => {
        alert('🚀 Deployed to Arduino!\n💡 Built-in LED will blink for testing');
        console.log('Deployed:', data);
    })
    .catch(err => {
        alert('❌ Deployment failed');
        console.error(err);
    });
}

// Generate Arduino code from config
function generateArduinoCode() {
    // This would generate actual Arduino C++ code based on the configuration
    // For now, just return the config as JSON
    return JSON.stringify(currentConfig);
}

// Clear all configuration
function clearAll() {
    if (!confirm('Are you sure you want to clear all devices?')) return;
    
    // Reset pin availability
    [...ARDUINO_PINS.digital, ...ARDUINO_PINS.analog].forEach(pin => {
        if (pin.pin !== 'D0' && pin.pin !== 'D1') {
            pin.available = true;
        }
    });
    
    currentConfig = {
        devices: [],
        pinAssignments: {},
        visualProgram: []
    };
    
    renderConfig();
    updateAvailableDevices();
    updateStats();
}

// Load saved configuration
function loadSavedConfig() {
    fetch('/api/config/load')
        .then(res => res.json())
        .then(config => {
            if (config && config.devices) {
                currentConfig = config;
                renderConfig();
                updateAvailableDevices();
                updateStats();
            }
        })
        .catch(err => console.log('No saved config found'));
}

// Select pin (for manual assignment)
let selectedPin = null;
function selectPin(pin) {
    console.log('Selected pin:', pin);
    selectedPin = pin;
}

// Drag and drop for visual programming
function allowDrop(ev) {
    ev.preventDefault();
}

function drop(ev) {
    ev.preventDefault();
    const data = ev.dataTransfer.getData("text");
    ev.target.appendChild(document.getElementById(data));
}

// Initialize on load
window.addEventListener('DOMContentLoaded', init);
