const firebaseConfig = {
    apiKey: "AIzaSyAlgqjJUeTN5f57PpcPckKxdOsLXLnXfzk",
    authDomain: "flipfocus-iot.firebaseapp.com",
    databaseURL: "https://flipfocus-iot-default-rtdb.asia-southeast1.firebasedatabase.app",
    projectId: "flipfocus-iot",
    storageBucket: "flipfocus-iot.firebasestorage.app",
    messagingSenderId: "622810947515",
    appId: "1:622810947515:web:dfab252ddf70ae7645ee9e",
    measurementId: "G-1MDGH7KKV7"
};

firebase.initializeApp(firebaseConfig);
// Explicitly pass the URL to ensure it routes to the Singapore (asia-southeast1) region
const database = firebase.database("https://flipfocus-iot-default-rtdb.asia-southeast1.firebasedatabase.app");
let currentListenerRef = null;

// Ensure we have an identity before trying to read data
function initializeAuth() {
    console.log("Attempting Anonymous Auth...");
    firebase.auth().signInAnonymously()
        .then(() => {
            console.log("✅ Authenticated anonymously. Ready to sync.");
            viewModeChanged();
        })
        .catch((error) => {
            console.error("❌ Authentication failed:", error.code, error.message);
            document.getElementById('status-text').innerHTML = '❌ <strong>Auth Error</strong> (' + error.code + ')';
        });
}

function formatTime(seconds) {
    if (seconds === undefined || seconds === null) {
        return '<span class="val">0</span><span class="lbl">s</span>';
    }

    const sec = parseInt(seconds, 10);
    if (isNaN(sec) || sec <= 0) {
        return '<span class="val">0</span><span class="lbl">s</span>';
    }

    const h = Math.floor(sec / 3600);
    const m = Math.floor((sec % 3600) / 60);
    const s = sec % 60;

    let html = "";
    if (h > 0) {
        html += `<span class="val">${h}</span><span class="lbl">h</span>`;
    }
    if (m > 0) {
        html += `<span class="val">${m}</span><span class="lbl">m</span>`;
    }
    if (s > 0 || (h === 0 && m === 0)) {
        html += `<span class="val">${s}</span><span class="lbl">s</span>`;
    }

    return html;
}

function updateCardMetrics(nodeData) {
    document.getElementById('time-1').innerHTML = formatTime(nodeData.assignment);
    document.getElementById('time-2').innerHTML = formatTime(nodeData.examprep);
    document.getElementById('time-3').innerHTML = formatTime(nodeData.corecoding);
    document.getElementById('time-4').innerHTML = formatTime(nodeData.totalbreak);
}

function highlightActiveCard(activeZoneIndex) {
    for (let i = 1; i <= 4; i++) {
        document.getElementById(`card-${i}`).classList.remove('active');
    }

    if (activeZoneIndex) {
        const targetCard = document.getElementById(`card-${activeZoneIndex}`);
        if (targetCard) targetCard.classList.add('active');
    }
}

function viewModeChanged() {
    const selector = document.getElementById('time-frame');
    const choice = selector.value;
    const statusIndicator = document.getElementById('status-indicator');
    const statusText = document.getElementById('status-text');

    if (currentListenerRef) {
        currentListenerRef.off();
        currentListenerRef = null;
    }

    if (choice === 'live') {
        statusIndicator.className = 'status-dot';
        statusText.innerHTML = 'Mode: <strong>Live Tracking Stream</strong>';

        currentListenerRef = database.ref('/profiles');
        console.log("Subscribing to live updates at /profiles...");
        
        currentListenerRef.on('value', (snapshot) => {
            const data = snapshot.val();
            console.log("Live Data Received:", data);
            if (data) {
                updateCardMetrics({
                    assignment: data.assignment,
                    examprep: data.examprep,
                    corecoding: data.corecoding,
                    totalbreak: data.totalbreak
                });
                highlightActiveCard(data.activeZone);
            }
        }, (error) => {
            console.error("Firebase Live Sync Error:", error);
        });
    } else {
        statusIndicator.className = 'status-dot historical';
        statusText.innerHTML = `Viewing Log: <strong>Archive (${choice})</strong>`;

        currentListenerRef = database.ref(`/profiles/history/${choice}`);
        console.log(`Subscribing to historical logs for: ${choice}...`);

        currentListenerRef.on('value', (snapshot) => {
            const data = snapshot.val();
            console.log(`Historical Data (${choice}):`, data);
            if (!data) {
                updateCardMetrics({ assignment: 0, examprep: 0, corecoding: 0, totalbreak: 0 });
            } else {
                updateCardMetrics(data);
            }
            highlightActiveCard(null);
        }, (error) => {
            console.error(`Firebase Historical Error (${choice}):`, error);
        });
    }
}

window.addEventListener('DOMContentLoaded', () => {
    const selector = document.getElementById('time-frame');
    selector.addEventListener('change', viewModeChanged);
    initializeAuth();
});
