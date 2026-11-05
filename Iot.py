# -*- coding: utf-8 -*-


import streamlit as st
from streamlit_autorefresh import st_autorefresh
import pyrebase
import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import datetime
import json

# =====================================================
# FIREBASE CONFIGURATION
# =====================================================
# Firebase web config is read from firebase_config.json (gitignored).
# Copy firebase_config.example.json and fill in your project's values.
import os
_cfg_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "firebase_config.json")
with open(_cfg_path, encoding="utf-8") as _f:
    FIREBASE_CONFIG = json.load(_f)

DEVICE_ID = "device_001"

# Notification thresholds
NOTIFY_LOW_THRESHOLD = 30
NOTIFY_HIGH_THRESHOLD = 70

try:
    firebase = pyrebase.initialize_app(FIREBASE_CONFIG)
    auth = firebase.auth()
    db = firebase.database()
except Exception as e:
    st.error(f"Firebase initialization error: {e}")

st.set_page_config(
    page_title="Smart Irrigation Pro",
    layout="wide",
    initial_sidebar_state="expanded"
)

# Custom CSS for Pleasant Agri Design
st.markdown(
    "<style>"
    "    @import url('https://fonts.googleapis.com/css2?family=Poppins:wght@300;400;500;600;700&family=Open+Sans:wght@400;600&display=swap');"
    ""
    "    :root {"
    "        --primary-green: #4CAF50;"
    "        --secondary-blue: #2196F3;"
    "        --light-bg: #f8f9fa;"
    "        --white: #ffffff;"
    "        --text-dark: #333333;"
    "        --text-muted: #666666;"
    "        --border-color: #e0e0e0;"
    "        --shadow-sm: 0 2px 4px rgba(0,0,0,0.05);"
    "        --shadow-md: 0 4px 6px rgba(0,0,0,0.1);"
    "    }"
    ""
    "    /* Main App Background */"
    "    .stApp {"
    "        background-color: var(--light-bg);"
    "        color: var(--text-dark);"
    "        font-family: 'Open Sans', sans-serif;"
    "    }"
    ""
    "    /* Typography */"
    "    h1, h2, h3, h4, h5, h6 {"
    "        font-family: 'Poppins', sans-serif;"
    "        color: var(--text-dark);"
    "        font-weight: 600;"
    "    }"
    ""
    "    p, label, span, div {"
    "        font-family: 'Open Sans', sans-serif;"
    "        color: var(--text-dark);"
    "    }"
    ""
    "    /* Buttons */"
    "    .stButton>button {"
    "        background-color: var(--white) !important;"
    "        border: 1px solid var(--primary-green) !important;"
    "        color: var(--primary-green) !important;"
    "        font-family: 'Poppins', sans-serif !important;"
    "        border-radius: 8px !important;"
    "        transition: all 0.3s ease !important;"
    "        box-shadow: var(--shadow-sm);"
    "        font-weight: 500;"
    "    }"
    "    "
    "    .stButton>button:hover {"
    "        background-color: var(--primary-green) !important;"
    "        color: var(--white) !important;"
    "        box-shadow: var(--shadow-md);"
    "        transform: translateY(-1px);"
    "    }"
    "    "
    "    /* Primary Buttons */"
    "    div[data-testid=\"stFormSubmitButton\"] > button, button[kind=\"primary\"] {"
    "        background-color: var(--primary-green) !important;"
    "        border-color: var(--primary-green) !important;"
    "        color: var(--white) !important;"
    "        box-shadow: var(--shadow-md);"
    "    }"
    "    div[data-testid=\"stFormSubmitButton\"] > button:hover, button[kind=\"primary\"]:hover {"
    "        background-color: #43A047 !important;"
    "        border-color: #43A047 !important;"
    "        box-shadow: 0 6px 8px rgba(0,0,0,0.15);"
    "    }"
    ""
    "    /* Inputs */"
    "    .stTextInput>div>div>input {"
    "        background-color: var(--white) !important;"
    "        border: 1px solid var(--border-color) !important;"
    "        color: var(--text-dark) !important;"
    "        font-family: 'Open Sans', sans-serif !important;"
    "        border-radius: 6px !important;"
    "        padding: 10px;"
    "    }"
    "    .stTextInput>div>div>input:focus {"
    "        border-color: var(--primary-green) !important;"
    "        box-shadow: 0 0 0 2px rgba(76, 175, 80, 0.2) !important;"
    "    }"
    "    "
    "    /* Tabs */"
    "    .stTabs [data-baseweb=\"tab-list\"] {"
    "        gap: 20px;"
    "        border-bottom: 2px solid var(--border-color);"
    "    }"
    "    .stTabs [data-baseweb=\"tab\"] {"
    "        background-color: transparent;"
    "        border: none;"
    "        color: var(--text-muted);"
    "        font-family: 'Poppins', sans-serif;"
    "        font-weight: 500;"
    "    }"
    "    .stTabs [aria-selected=\"true\"] {"
    "        color: var(--primary-green) !important;"
    "        border-bottom: 3px solid var(--primary-green) !important;"
    "    }"
    ""
    "    /* Cards */"
    "    .metric-card {"
    "        background: var(--white);"
    "        border: 1px solid var(--border-color);"
    "        box-shadow: var(--shadow-sm);"
    "        border-radius: 12px;"
    "        padding: 20px;"
    "        transition: transform 0.2s;"
    "    }"
    "    .metric-card:hover {"
    "        transform: translateY(-2px);"
    "        box-shadow: var(--shadow-md);"
    "    }"
    "    "
    "    /* Sidebar */"
    "    section[data-testid=\"stSidebar\"] {"
    "        background-color: var(--white);"
    "        border-right: 1px solid var(--border-color);"
    "    }"
    "    section[data-testid=\"stSidebar\"] h1, "
    "    section[data-testid=\"stSidebar\"] h2, "
    "    section[data-testid=\"stSidebar\"] h3 {"
    "        color: var(--text-dark) !important;"
    "    }"
    ""
    "    /* Alerts */"
    "    .stAlert {"
    "        background-color: var(--white) !important;"
    "        border: none;"
    "        box-shadow: var(--shadow-sm);"
    "        color: var(--text-dark) !important;"
    "        border-left: 4px solid var(--primary-green);"
    "    }"
    "</style>",
    unsafe_allow_html=True
)

# JavaScript for browser notifications
notification_script = (
    "<script>"
    "function requestNotificationPermission() {"
    "    if ('Notification' in window && Notification.permission === 'default') {"
    "        Notification.requestPermission();"
    "    }"
    "}"
    "function showNotification(title, body, icon) {"
    "    if ('Notification' in window && Notification.permission === 'granted') {"
    "        new Notification(title, {"
    "            body: body,"
    "            icon: icon || '💧',"
    "            badge: '🌱',"
    "            vibrate: [200, 100, 200],"
    "            requireInteraction: true"
    "        });"
    "    }"
    "}"
    "requestNotificationPermission();"
    "</script>"
)

# =====================================================
# NOTIFICATION FUNCTIONS
# =====================================================

def show_browser_notification(title, message, notification_type="info"):

    icons = {
        "critical": "🚨",
        "warning": "⚠️",
        "info": "ℹ️",
        "success": "✅"
    }
    
    icon = icons.get(notification_type, "💧")
    
    js_code = (
        "<script>"
        "    if ('Notification' in window && Notification.permission === 'granted') {"
        f"        new Notification('{title}', {{"
        f"            body: '{message}',"
        f"            icon: '{icon}',"
        "            badge: '🌱',"
        "            vibrate: [200, 100, 200],"
        "            requireInteraction: true"
        "        });"
        "    }"
        "</script>"
    )
    st.components.v1.html(js_code, height=0)

def check_and_notify(moisture, last_moisture, pump_status):

    notifications = []
    
    # Initialize notification tracking in session state
    if "last_notification_time" not in st.session_state:
        st.session_state.last_notification_time = {}
    
    if "last_notified_state" not in st.session_state:
        st.session_state.last_notified_state = None
    
    current_time = datetime.datetime.now()
    
    # Prevent notification spam (minimum 5 minutes between same type)
    def can_notify(notify_type):
        if notify_type not in st.session_state.last_notification_time:
            return True
        
        last_time = st.session_state.last_notification_time[notify_type]
        time_diff = (current_time - last_time).total_seconds()
        return time_diff > 300  # 5 minutes
    
    # Check for low moisture (< 30%)
    if moisture < NOTIFY_LOW_THRESHOLD:
        if st.session_state.last_notified_state != "low" and can_notify("low"):
            notifications.append({
                "type": "critical",
                "title": "🚨 LOW MOISTURE ALERT",
                "message": f"Soil moisture is {moisture}% (below {NOTIFY_LOW_THRESHOLD}%). Turn ON pump immediately!",
                "action": "Turn ON pump to water the plants"
            })
            st.session_state.last_notification_time["low"] = current_time
            st.session_state.last_notified_state = "low"
            
            # Show browser notification
            show_browser_notification(
                "🚨 LOW MOISTURE ALERT",
                f"Soil moisture at {moisture}% - Turn ON pump!",
                "critical"
            )
    
    # Check for high moisture (> 70%)
    elif moisture > NOTIFY_HIGH_THRESHOLD:
        if st.session_state.last_notified_state != "high" and can_notify("high"):
            notifications.append({
                "type": "warning",
                "title": "💧 HIGH MOISTURE ALERT",
                "message": f"Soil moisture is {moisture}% (above {NOTIFY_HIGH_THRESHOLD}%). Turn OFF pump to prevent overwatering!",
                "action": "Turn OFF pump to avoid overwatering"
            })
            st.session_state.last_notification_time["high"] = current_time
            st.session_state.last_notified_state = "high"
            
            # Show browser notification
            show_browser_notification(
                "💧 HIGH MOISTURE ALERT",
                f"Soil moisture at {moisture}% - Turn OFF pump!",
                "warning"
            )
    
    # Check if moisture returned to normal range
    elif NOTIFY_LOW_THRESHOLD <= moisture <= NOTIFY_HIGH_THRESHOLD:
        if st.session_state.last_notified_state in ["low", "high"] and can_notify("normal"):
            notifications.append({
                "type": "success",
                "title": "✅ MOISTURE NORMALIZED",
                "message": f"Soil moisture is {moisture}% - Back to optimal range!",
                "action": "No action needed"
            })
            st.session_state.last_notification_time["normal"] = current_time
            st.session_state.last_notified_state = "normal"
            
            # Show browser notification
            show_browser_notification(
                "✅ MOISTURE NORMALIZED",
                f"Soil moisture at {moisture}% - Optimal range!",
                "success"
            )
    
    return notifications

def log_notification(notification_data):

    try:
        timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat().replace("+00:00", "Z")
        db.child("notifications").child(DEVICE_ID).push({
            "type": notification_data["type"],
            "title": notification_data["title"],
            "message": notification_data["message"],
            "timestamp": timestamp,
            "read": False
        })
    except Exception as e:
        st.error(f"Failed to log notification: {e}")

def get_recent_notifications(limit=10):

    try:
        data = db.child("notifications").child(DEVICE_ID).get().val()
        if not data:
            return []
        
        notifications = []
        for key, val in data.items():
            try:
                ts_str = val.get("timestamp", "")
                ts = datetime.datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
                notifications.append({
                    "id": key,
                    "timestamp": ts,
                    **val
                })
            except:
                continue
        
        # Sort by timestamp (newest first) and limit
        return sorted(notifications, key=lambda x: x["timestamp"], reverse=True)[:limit]
    except:
        return []

def mark_notification_read(notification_id):

    try:
        db.child("notifications").child(DEVICE_ID).child(notification_id).update({"read": True})
        return True
    except:
        return False

def clear_all_notifications():

    try:
        db.child("notifications").child(DEVICE_ID).remove()
        return True
    except:
        return False

# =====================================================
# helper functions (keeping existing ones)
# =====================================================

def login(email, password):

    try:
        return auth.sign_in_with_email_and_password(email, password)
    except Exception as e:
        error_msg = str(e)
        if "INVALID_PASSWORD" in error_msg or "INVALID_LOGIN_CREDENTIALS" in error_msg:
            st.error("🔒 Invalid email or password")
        elif "EMAIL_NOT_FOUND" in error_msg:
            st.error("📧 Email not found. Please sign up first.")
        else:
            st.error(f"❌ Login failed: {error_msg}")
        return None

def signup(email, password):

    try:
        return auth.create_user_with_email_and_password(email, password)
    except Exception as e:
        error_msg = str(e)
        if "EMAIL_EXISTS" in error_msg:
            st.error("📧 Email already exists. Please login instead.")
        elif "WEAK_PASSWORD" in error_msg:
            st.error("🔐 Password should be at least 6 characters")
        else:
            st.error(f"❌ Sign-up failed: {error_msg}")
        return None

def get_device_data():

    try:
        data = db.child("devices").child(DEVICE_ID).get().val()
        return data if data else {}
    except Exception as e:
        st.error(f"❌ Error fetching device data: {e}")
        return {}

def update_pump_status(status):

    try:
        timestamp = datetime.datetime.now(datetime.timezone.utc).isoformat().replace("+00:00", "Z")
        
        # Update device status
        db.child("devices").child(DEVICE_ID).child("actuators").child("pump").update({
            "status": status,
            "mode": "MANUAL",
            "lastChanged": timestamp
        })
        
        # Log to history
        db.child("history").child(DEVICE_ID).child("pump").push({
            "value": status,
            "trigger": "MANUAL",
            "timestamp": timestamp
        })
        return True
    except Exception as e:
        st.error(f"❌ Failed to update pump: {e}")
        return False

def update_settings(auto_mode, threshold_low, threshold_high):

    try:
        # Update settings
        db.child("devices").child(DEVICE_ID).child("settings").update({
            "autoMode": auto_mode,
            "thresholds": {
                "low": threshold_low,
                "high": threshold_high
            }
        })
        
        # Update pump mode
        db.child("devices").child(DEVICE_ID).child("actuators").child("pump").update({
            "mode": "AUTO" if auto_mode else "MANUAL"
        })
        return True
    except Exception as e:
        st.error(f"❌ Failed to update settings: {e}")
        return False

def get_historical_data(data_type="moisture", hours=24):

    try:
        data = db.child("history").child(DEVICE_ID).child(data_type).get().val()
        if not data:
            return []
        
        cutoff = datetime.datetime.now(datetime.timezone.utc) - datetime.timedelta(hours=hours)
        records = []
        
        for key, val in data.items():
            try:
                ts_str = val.get("timestamp", "")
                ts = datetime.datetime.fromisoformat(ts_str.replace("Z", "+00:00"))
                
                if ts > cutoff:
                    records.append({
                        "timestamp": ts,
                        "value": val.get("value"),
                        **{k: v for k, v in val.items() if k not in ["timestamp", "value"]}
                    })
            except Exception:
                continue
        
        return sorted(records, key=lambda x: x["timestamp"])
    except Exception:
        return []

def get_condition_from_moisture(moisture):

    if moisture < 25:
        return "Very Dry", "🔴", "#F44336", "⚠ Critical"
    elif moisture < 45:
        return "Dry", "🟠", "#FF9800", "⚡ Action Needed"
    elif moisture < 70:
        return "Moist", "🟢", "#4CAF50", "✅ Optimal"
    else:
        return "Wet", "🔵", "#2196F3", "💧 Saturated"

def get_ai_recommendation(moisture, pump_status):

    if moisture < 20:
        return "🚨", "CRITICAL", "Soil critically dry! Water immediately for 15-20 minutes.", "#F44336"
    elif moisture < 30:
        return "⚠", "URGENT", "Soil is dry. Water now for 10-15 minutes.", "#FF9800"
    elif moisture < 45:
        return "💡", "SUGGESTED", "Moisture low. Short watering (5-10 min) recommended.", "#FFC107"
    elif moisture < 70:
        return "✅", "OPTIMAL", "Soil healthy! No watering needed today.", "#4CAF50"
    elif moisture < 85:
        return "💧", "GOOD", "Soil adequately moist. Skip watering.", "#2196F3"
    else:
        return "🛑", "WARNING", "Soil saturated! Risk of overwatering. Stop pump!", "#9C27B0"

def calculate_pump_runtime(pump_history):

    if not pump_history:
        return 0
    
    total_seconds = 0
    last_on_time = None
    
    sorted_history = sorted(pump_history, key=lambda x: x["timestamp"])
    
    for record in sorted_history:
        if record["value"] == "ON":
            last_on_time = record["timestamp"]
        elif record["value"] == "OFF" and last_on_time:
            duration = (record["timestamp"] - last_on_time).total_seconds()
            total_seconds += duration
            last_on_time = None
    
    # If pump is currently on
    if last_on_time:
        duration = (datetime.datetime.now(datetime.timezone.utc) - last_on_time).total_seconds()
        total_seconds += duration
    
    return total_seconds

def format_runtime(seconds):

    if seconds < 60:
        return f"{int(seconds)}s"
    elif seconds < 3600:
        return f"{int(seconds/60)}m {int(seconds%60)}s"
    else:
        hours = int(seconds/3600)
        minutes = int((seconds%3600)/60)
        return f"{hours}h {minutes}m"

# =====================================================
# PAGES
# =====================================================

def landing_page():

    # Hero Section
    st.markdown(
        "<div style='text-align:center;padding:80px 20px 60px 20px;margin-bottom:40px;'>"
        "<h1 style='font-size:4em;margin-bottom:15px;color:#2c3e50;font-weight:700;'>"
        "Smart <span style='color:#4CAF50;'>Irrigation</span>"
        "</h1>"
        "<p style='font-size:1.3em;color:#666;max-width:700px;margin:0 auto;font-weight:400;'>"
        "Intelligent water management for a sustainable future."
        "</p>"
        "</div>",
        unsafe_allow_html=True,
    )
    
    # CTA Button
    c1, c2, c3 = st.columns([1, 1, 1])
    with c2:
        if st.button("Get Started", use_container_width=True, type="primary"):
            st.session_state.page = "login"
            st.rerun()

    st.markdown("<br><br>", unsafe_allow_html=True)
    
    # Key Features Grid
    col1, col2, col3 = st.columns(3)
    
    features = [
        ("🌿", "Healthy Growth", "Real-time monitoring for optimal plant health."),
        ("💧", "Smart Watering", "Automated irrigation based on precise moisture levels."),
        ("📊", "Data Insights", "Track usage and environmental trends over time.")
    ]
    
    for col, (icon, title, desc) in zip([col1, col2, col3], features):
        with col:
            st.markdown(
                f"<div class='metric-card' style='height:250px;text-align:center;padding:25px;display:flex;flex-direction:column;justify-content:center;align-items:center;'>"
                f"<div style='font-size:3em;margin-bottom:15px;color:#4CAF50;'>{icon}</div>"
                f"<h3 style='color:#333;margin-bottom:10px;font-size:1.2em;'>{title}</h3>"
                f"<p style='color:#666;font-size:0.9em;line-height:1.6;'>{desc}</p>"
                "</div>",
                unsafe_allow_html=True
            )
    
    st.markdown("<br><br><br>", unsafe_allow_html=True)
    
    # Footer
    st.markdown(
        "<div style='text-align:center;border-top:1px solid #e0e0e0;padding-top:20px;color:#888;font-size:0.8em;'>"
        "© 2025 Smart Irrigation Pro. All rights reserved."
        "</div>",
        unsafe_allow_html=True
    )

def login_page():

    # Inject notification script
    st.components.v1.html(notification_script, height=0)
    
    st.markdown("<br>", unsafe_allow_html=True)
    
    # Grid for centering
    c1, c2, c3 = st.columns([1, 1.5, 1])
    
    with c2:
        st.markdown(
            "<div class='metric-card' style='text-align:center;padding:40px;background:white;'>"
            "<h2 style='color:#4CAF50;margin-bottom:10px;'>Welcome Back</h2>"
            "<p style='color:#666;margin-bottom:30px;'>Sign in to access your irrigation controls</p>"
            "</div>"
            "<br>",
            unsafe_allow_html=True
        )
        
        # Tabs for Auth
        tab1, tab2 = st.tabs(["Sign In", "Register"])
        
        with tab1:
            with st.form("login_form"):
                st.text_input("Email Address", key="login_email")
                st.text_input("Password", type="password", key="login_pass")
                
                st.markdown("<br>", unsafe_allow_html=True)
                submitted = st.form_submit_button("Sign In", use_container_width=True, type="primary")
                
                if submitted:
                    email = st.session_state.login_email
                    password = st.session_state.login_pass
                    
                    if email and password:
                        with st.spinner("Authenticating..."):
                            user = login(email, password)
                            if user:
                                st.session_state.user = user
                                st.session_state.page = "dashboard"
                                st.success("Login successful!")
                                st.rerun()
                    else:
                        st.warning("Please fill in all fields.")
        
        with tab2:
            with st.form("signup_form"):
                st.text_input("Email Address", key="signup_email")
                st.text_input("Create Password", type="password", key="signup_pass")
                st.text_input("Confirm Password", type="password", key="confirm_pass")
                
                st.markdown("<br>", unsafe_allow_html=True)
                submitted = st.form_submit_button("Create Account", use_container_width=True, type="primary")
                
                if submitted:
                    email = st.session_state.signup_email
                    password = st.session_state.signup_pass
                    confirm = st.session_state.confirm_pass
                    
                    if email and password and confirm:
                        if password != confirm:
                            st.error("Passwords do not match")
                        elif len(password) < 6:
                            st.error("Password must be at least 6 characters")
                        else:
                            with st.spinner("Creating account..."):
                                user = signup(email, password)
                                if user:
                                    st.success("Account created! Please sign in.")
                    else:
                        st.warning("Please fill in all fields.")

        # Back Button
        st.markdown("<br>", unsafe_allow_html=True)
        if st.button("← Back to Home", use_container_width=True):
            st.session_state.page = "home"
            st.rerun()

def dashboard_page():
    if "user" not in st.session_state:
        st.warning("⚠ Please sign in to access the dashboard.")
        if st.button("← Go to Login"):
            st.session_state.page = "login"
            st.rerun()
        return

    # Inject notification script
    st.components.v1.html(notification_script, height=0)

    # Auto-refresh
    count = st_autorefresh(interval=5000, key="refresh")

    # Fetch data
    device_data = get_device_data()
    if not device_data:
        st.error("❌ Connection lost. Please check internet connection.")
        return

    sensor_data = device_data.get("sensors", {})
    pump_data = device_data.get("actuators", {}).get("pump", {})
    settings_data = device_data.get("settings", {})
    info_data = device_data.get("info", {})

    # Extract values
    moisture = int(sensor_data.get("moisture", 0))
    pump_status = pump_data.get("status", "OFF")
    pump_mode = pump_data.get("mode", "AUTO")
    auto_mode = settings_data.get("autoMode", True)
    thresholds = settings_data.get("thresholds", {})
    threshold_low = thresholds.get("low", 30)
    threshold_high = thresholds.get("high", 70)
    last_seen = info_data.get("lastSeen", "")
    location = info_data.get("location", "Sector 7")
    device_name = info_data.get("name", "Sensor Unit Alpha")

    # Track last moisture
    if "last_moisture" not in st.session_state:
        st.session_state.last_moisture = moisture
    
    last_moisture = st.session_state.last_moisture
    new_notifications = check_and_notify(moisture, last_moisture, pump_status)
    for notification in new_notifications:
        log_notification(notification)
    st.session_state.last_moisture = moisture

    # ===== SIDEBAR (SYSTEM CONFIG) =====
    with st.sidebar:
        st.markdown(
            "<div style='text-align:center;margin-bottom:20px;border-bottom:1px solid #e0e0e0;padding-bottom:10px;'>"
            "<h3 style='color:#4CAF50;margin:0;'>Settings</h3>"
            "<p style='font-size:0.8em;color:#999;'>Version 2.4.1</p>"
            "</div>",
            unsafe_allow_html=True
        )
        
        # Notifications
        with st.expander(">> ALERT LOG", expanded=True):
            recent_notifications = get_recent_notifications(limit=5)
            unread_count = sum(1 for n in recent_notifications if not n.get("read", False))
            
            if unread_count > 0:
                st.markdown(f"<span class='notification-badge' style='background:#d32f2f;color:white;font-family:Orbitron;'>! {unread_count} ACTIVE ALERTS</span>", unsafe_allow_html=True)
            
            if recent_notifications:
                for notif in recent_notifications[:3]:
                    time_ago = (datetime.datetime.now(datetime.timezone.utc) - notif["timestamp"]).total_seconds()
                    
                    if time_ago < 60: time_str = f"{int(time_ago)}s"
                    elif time_ago < 3600: time_str = f"{int(time_ago/60)}m"
                    else: time_str = f"{int(time_ago/3600)}h"
                    
                    notif_type = notif.get("type", "info")
                    color = "#d32f2f" if notif_type == "critical" else "#f57c00" if notif_type == "warning" else "#39ff14"
                    
                    st.markdown(
                        "<div style='padding:8px;margin:5px 0;background:rgba(0,0,0,0.5);border-left:2px solid " + color + ";font-size:0.8em;'>"
                        f"<strong style='color:{color};font-family:Orbitron;'>[{notif.get('title', 'ALERT')}]</strong><br>"
                        f"<span style='color:#888;'>T-{time_str}</span>"
                        "</div>",
                        unsafe_allow_html=True
                    )
                
                if st.button("CLEAR LOG", key="clear_notif"):
                    clear_all_notifications()
                    st.rerun()
            else:
                st.caption("SYSTEM NOMINAL. NO ALERTS.")
        
        # Automation Settings
        st.markdown("<br>", unsafe_allow_html=True)
        st.markdown("<h5 style='color:#00ffff;'>AUTOMATION PROTOCOLS</h5>", unsafe_allow_html=True)
        
        with st.container(border=True):
            new_auto_mode = st.toggle("AUTO-SEQUENCE", value=auto_mode)
            
            if new_auto_mode:
                st.caption("HYDRATION TRIGGERS")
                new_threshold_low = st.slider("LOW THRESHOLD %", 10, 50, threshold_low)
                new_threshold_high = st.slider("HIGH THRESHOLD %", 50, 90, threshold_high)
                
                if st.button(">> UPLOAD PARAMETERS <<", type="primary", use_container_width=True):
                    update_settings(new_auto_mode, new_threshold_low, new_threshold_high)
                    st.success("PARAMETERS UPDATED")
                    st.rerun()
            else:
                st.caption("MANUAL OVERRIDE ACTIVE")
                if st.button("ENGAGE AUTO-SEQUENCE", use_container_width=True):
                    update_settings(True, threshold_low, threshold_high)
                    st.rerun()
        
        with st.expander(">> DATA STREAM", expanded=False):
            time_range = st.selectbox(
                "HISTORY BUFFER",
                ["Last 1 Hour", "Last 6 Hours", "Last 12 Hours", "Last 24 Hours", "Last 3 Days", "Last Week"],
                index=3
            )
            
            hours_map = {
                "Last 1 Hour": 1,
                "Last 6 Hours": 6,
                "Last 12 Hours": 12,
                "Last 24 Hours": 24,
                "Last 3 Days": 72,
                "Last Week": 168
            }
            selected_hours = hours_map[time_range]
            
            show_stats = st.checkbox("DISPLAY METRICS", value=True)
            show_pie = st.checkbox("DISPLAY DISTRIBUTION", value=True)
        
        st.markdown("---")
        if st.button("TERMINATE SESSION", use_container_width=True):
            st.session_state.clear()
            st.session_state.page = "home"
            st.rerun()
        
        # System info
        st.markdown("---")
        st.caption(f"REFRESH CYCLE: {count} | LINK: SECURE")

    # ===== MAIN HUD DASHBOARD =====
    
    # Header HUD element
    # Header HUD element
    st.markdown(
        "<div style='display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #e0e0e0;padding-bottom:10px;margin-bottom:20px;'>"
        "<div>"
        "<h2 style='margin:0;font-size:1.8em;color:#333;'>Live Telemetry</h2>"
        f"<span style='color:#666;letter-spacing:1px;'>TARGET: {location} | ID: {device_name}</span>"
        "</div>"
        "<div style='text-align:right;'>"
        f"<div style='font-size:1.5em;color:#4CAF50;font-weight:600;'>{datetime.datetime.now().strftime('%H:%M:%S')}</div>"
        "<div style='color:#888;font-size:0.8em;'>SYSTEM TIME</div>"
        "</div>"
        "</div>",
        unsafe_allow_html=True
    )
    
    # Active Notifications Banner
    if new_notifications:
        for n in new_notifications:
             st.markdown(
                 "<div style='background:#ffebee;border:1px solid #ef5350;color:#c62828;padding:15px;margin-bottom:10px;display:flex;align-items:center;border-radius:8px;'>"
                 "<span style='font-size:1.5em;margin-right:15px;'>&#9888; Warning</span>"
                 "<div>"
                 f"<b>{n['title']}</b><br>"
                 f"{n['message']}"
                 "</div>"
                 "</div>",
                 unsafe_allow_html=True
             )
    
    # Device Connectivity Status
    try:
        last_seen_dt = datetime.datetime.fromisoformat(last_seen.replace("Z", "+00:00"))
        time_diff = datetime.datetime.now(datetime.timezone.utc) - last_seen_dt
        
        if time_diff.total_seconds() < 60:
            status_html = "<span style='color:#4CAF50;font-weight:600;'>● Online</span>"
        elif time_diff.total_seconds() < 300:
             status_html = f"<span style='color:#FFC107;font-weight:600;'>● Latency ({int(time_diff.total_seconds())}s)</span>"
        else:
            status_html = "<span style='color:#F44336;font-weight:600;'>● Offline</span>"
    except:
        status_html = "<span style='color:#999;font-weight:600;'>● No Signal</span>"

    st.markdown(f"<div style='text-align:right;margin-bottom:10px;font-size:0.8em;color:#666;'>Status: {status_html}</div>", unsafe_allow_html=True)


    # Top Metrics Grid (HUD Cards)
    col1, col2, col3, col4 = st.columns(4)
    
    condition, icon, color, status_text = get_condition_from_moisture(moisture)
    
    with col1:
        st.markdown(
            "<div class='metric-card' style='text-align:center;padding:20px;'>"
            "<div style='color:#666;font-size:0.8em;margin-bottom:5px;text-transform:uppercase;'>Soil Moisture</div>"
            f"<h1 style='font-size:3em;margin:0;color:{color};font-weight:700;'>{moisture}%</h1>"
            "</div>",
            unsafe_allow_html=True
        )
    
    with col2:
        st.markdown(
            "<div class='metric-card' style='text-align:center;padding:20px;'>"
            "<div style='color:#666;font-size:0.8em;margin-bottom:5px;text-transform:uppercase;'>Condition</div>"
            f"<h2 style='margin:5px 0;color:{color};font-size:1.6em;font-weight:600;'>{condition}</h2>"
            f"<div style='font-size:0.8em;color:#888;'>{status_text}</div>"
            "</div>",
            unsafe_allow_html=True
        )
    
    with col3:
        pump_text_color = "#4CAF50" if pump_status == "ON" else "#666"
        st.markdown(
            "<div class='metric-card' style='text-align:center;padding:20px;'>"
            "<div style='color:#666;font-size:0.8em;margin-bottom:5px;text-transform:uppercase;'>Pump Status</div>"
            f"<h2 style='margin:5px 0;color:{pump_text_color};font-size:2em;font-weight:600;'>{pump_status}</h2>"
            "</div>",
            unsafe_allow_html=True
        )
    
    with col4:
        mode_color = "#2196F3" if pump_mode == "AUTO" else "#FF9800"
        st.markdown(
            "<div class='metric-card' style='text-align:center;padding:20px;'>"
            "<div style='color:#666;font-size:0.8em;margin-bottom:5px;text-transform:uppercase;'>Control Mode</div>"
            f"<h2 style='margin:5px 0;color:{mode_color};font-size:1.6em;font-weight:600;'>{pump_mode}</h2>"
            "</div>",
            unsafe_allow_html=True
        )

    st.markdown("<br>", unsafe_allow_html=True)

    # Main Content Area
    left_panel, right_panel = st.columns([1, 2])

    # ===== LEFT PANEL (CONTROLS & AI) =====
    with left_panel:
        # AI Recommendation HUD
        ai_icon, ai_level, ai_msg, ai_color = get_ai_recommendation(moisture, pump_status)
        st.markdown(
            f"<div class='metric-card' style='padding:20px;border-left:4px solid {ai_color};'>"
            "<div style='display:flex;justify-content:space-between;'>"
            f"<span style='color:{ai_color};font-weight:600;'>AI ANALYSIS</span>"
            f"<span style='color:#666;font-size:0.8em;'>Confidence: {ai_level}</span>"
            "</div>"
            f"<h3 style='margin:10px 0;color:#333;font-size:1.1em;'>{ai_msg}</h3>"
            "</div>",
            unsafe_allow_html=True
        )
        
        st.markdown("<br>", unsafe_allow_html=True)
        
        # Manual Override
        st.markdown("<h4 style='border-bottom:1px solid #e0e0e0;padding-bottom:5px;color:#333;'>Manual Override</h4>", unsafe_allow_html=True)
        
        if auto_mode:
            st.info("ℹ Auto-sequence engaged. Manual input will suspend automation.")
        
        col_btn1, col_btn2 = st.columns(2)
        
        with col_btn1:
            if st.button("Activate Pump", use_container_width=True, type="primary", disabled=(pump_status == "ON")):
                with st.spinner("Activating..."):
                    if update_pump_status("ON"):
                        st.success("Pump Active")
                        st.rerun()
        
        with col_btn2:
            if st.button("Deactivate Pump", use_container_width=True, disabled=(pump_status == "OFF")):
                with st.spinner("Deactivating..."):
                    if update_pump_status("OFF"):
                        st.success("Pump Inactive")
                        st.rerun()
        
        # Statistics HUD
        st.markdown("<br>", unsafe_allow_html=True)
        st.markdown("<h4 style='border-bottom:1px solid #e0e0e0;padding-bottom:5px;color:#333;'>Performance Stats</h4>", unsafe_allow_html=True)
        
        pump_history = get_historical_data("pump", selected_hours)
        runtime_seconds = calculate_pump_runtime(pump_history)
        
        st.markdown(
            "<div style='display:flex;justify-content:space-between;border-bottom:1px dashed #e0e0e0;padding:5px 0;'>"
            f"<span style='color:#666;'>Runtime (Last {time_range})</span>"
            f"<span style='color:#333;font-weight:600;'>{format_runtime(runtime_seconds)}</span>"
            "</div>",
            unsafe_allow_html=True
        )
        
        recent_moisture = get_historical_data("moisture", hours=1)
        if len(recent_moisture) > 1:
            recent_values = [int(r["value"]) for r in recent_moisture if r["value"]]
            if recent_values:
                avg_moisture = sum(recent_values) / len(recent_values)
                trend = recent_values[-1] - recent_values[0]
                trend_color = "#4CAF50" if trend > 0 else "#F44336" if trend < 0 else "#888"
                trend_symbol = "↑" if trend > 0 else "↓" if trend < 0 else "→"
                
                st.markdown(
                    "<div style='display:flex;justify-content:space-between;padding:5px 0;'>"
                    "<span style='color:#666;'>1-hr Average</span>"
                    f"<span style='color:#333;font-weight:600;'>{avg_moisture:.1f}%</span>"
                    "</div>"
                    "<div style='display:flex;justify-content:space-between;padding:5px 0;'>"
                    "<span style='color:#666;'>Trend</span>"
                    f"<span style='color:{trend_color};font-weight:600;'>{trend_symbol} {abs(trend):.1f}%</span>"
                    "</div>",
                    unsafe_allow_html=True
                )

    # ===== RIGHT PANEL (CHARTS) =====
    # ===== RIGHT PANEL (CHARTS) =====
    with right_panel:
        st.markdown("<h4 style='border-bottom:1px solid #e0e0e0;padding-bottom:5px;color:#333;'>Analytics Visualization</h4>", unsafe_allow_html=True)
        
        moisture_history = get_historical_data("moisture", selected_hours)
        
        if moisture_history:
            df_moisture = pd.DataFrame(moisture_history)
            df_moisture["value"] = pd.to_numeric(df_moisture["value"], errors="coerce")
            df_moisture = df_moisture.dropna(subset=["value"])
            
            if not df_moisture.empty:
                # Plotly Chart with Light Theme
                fig = make_subplots(
                    rows=2, cols=1,
                    subplot_titles=("Moisture Levels (%)", "Pump Activity"),
                    row_heights=[0.7, 0.3],
                    vertical_spacing=0.15
                )
                
                # Light Theme Template Updates
                layout_update = dict(
                    plot_bgcolor='var(--white)',
                    paper_bgcolor='var(--white)',
                    font=dict(color='#333', family="Open Sans"),
                )
                
                # Moisture Line - Green
                fig.add_trace(
                    go.Scatter(
                        x=df_moisture["timestamp"],
                        y=df_moisture["value"],
                        mode="lines+markers",
                        name="Moisture",
                        line=dict(color="#4CAF50", width=3, shape="spline"),
                        marker=dict(size=6, color="#4CAF50", line=dict(width=1, color="#fff")),
                        fill='tozeroy',
                        fillcolor='rgba(76, 175, 80, 0.1)',
                        hovertemplate="%{y:.1f}%<br>%{x}<extra></extra>"
                    ),
                    row=1, col=1
                )
                
                # Threshold Lines
                fig.add_hline(y=NOTIFY_LOW_THRESHOLD, line_dash="dot", line_color="#F44336", opacity=0.7, row=1, col=1, annotation_text="Crit. Low")
                fig.add_hline(y=NOTIFY_HIGH_THRESHOLD, line_dash="dot", line_color="#2196F3", opacity=0.7, row=1, col=1, annotation_text="Saturation")
                
                # Pump Activity
                pump_history = get_historical_data("pump", selected_hours)
                if pump_history:
                    df_pump = pd.DataFrame(pump_history)
                    df_pump["status_num"] = df_pump["value"].apply(lambda x: 1 if x == "ON" else 0)
                    
                    colors = df_pump["trigger"].apply(lambda x: "#4CAF50" if x == "AUTO" else "#FF9800")
                    
                    fig.add_trace(
                        go.Scatter(
                            x=df_pump["timestamp"],
                            y=df_pump["status_num"],
                            mode="markers+lines",
                            name="Pump",
                            line=dict(color="#2196F3", width=2, shape='hv'),
                            marker=dict(size=8, color=colors),
                            hovertemplate="%{x}<br>Status: %{y}<extra></extra>"
                        ),
                        row=2, col=1
                    )

                # Styling Axes
                fig.update_xaxes(showgrid=True, gridcolor='#eee', zeroline=False, row=1, col=1)
                fig.update_yaxes(showgrid=True, gridcolor='#eee', zeroline=False, row=1, col=1)
                fig.update_xaxes(showgrid=True, gridcolor='#eee', zeroline=False, row=2, col=1)
                fig.update_yaxes(showgrid=False, tickvals=[0, 1], ticktext=["OFF", "ON"], row=2, col=1)
                
                fig.update_layout(height=600, margin=dict(l=0, r=0, t=30, b=0), showlegend=False, **layout_update)
                
                st.plotly_chart(fig, use_container_width=True)
                
                # Condition Pie Chart
                if show_pie:
                    st.markdown("<br><h4 style='border-bottom:1px solid #e0e0e0;padding-bottom:5px;color:#333;'>Distribution Analysis</h4>", unsafe_allow_html=True)
                    col_p1, col_p2 = st.columns([1, 1])
                    
                    with col_p1:
                         conditions = df_moisture["value"].apply(lambda x: get_condition_from_moisture(x)[0])
                         condition_counts = conditions.value_counts()
                         
                         pie_colors = [{"Very Dry": "#ff5252", "Dry": "#ffb74d", "Moist": "#66bb6a", "Wet": "#42a5f5"}.get(x, "#bdbdbd") for x in condition_counts.index]
                         
                         fig_pie = go.Figure(data=[go.Pie(
                             labels=condition_counts.index, 
                             values=condition_counts.values,
                             marker=dict(colors=pie_colors, line=dict(color='#fff', width=2)),
                             hole=0.6,
                             textinfo='percent'
                         )])
                         fig_pie.update_layout(height=250, margin=dict(l=0,r=0,t=0,b=0), **layout_update, showlegend=False)
                         st.plotly_chart(fig_pie, use_container_width=True)
                    
                    with col_p2:
                        st.markdown("<br>", unsafe_allow_html=True)
                        for condition, count in condition_counts.items():
                             pct = (count/len(conditions))*100
                             clr = {"Very Dry": "#ff5252", "Dry": "#ffb74d", "Moist": "#66bb6a", "Wet": "#42a5f5"}.get(condition, "#bdbdbd")
                             st.markdown(f"<div style='border-left:3px solid {clr};padding-left:10px;margin-bottom:5px;font-size:0.9em;color:#666;'>{condition}: <b>{pct:.1f}%</b></div>", unsafe_allow_html=True)

            else:
                st.info("Awaiting data stream...")
        else:
             st.info("Initializing data link...")
    
    # Show all notifications modal (Overlay Style)
    if st.session_state.get("show_all_notifications", False):
        with st.expander(">> FULL SYSTEM LOG", expanded=True):
            all_notifications = get_recent_notifications(limit=20)
            if all_notifications:
                for notif in all_notifications:
                    time_str = notif["timestamp"].strftime("%H:%M:%S")
                    st.markdown(f"<div style='border-bottom:1px solid #e0e0e0;padding:5px;font-size:0.9em;'><span style='color:#666;font-weight:600;'>[{time_str}]</span> {notif.get('message','')}</div>", unsafe_allow_html=True)
                
                if st.button("CLOSE LOG", use_container_width=True):
                    st.session_state.show_all_notifications = False
                    st.rerun()

# =====================================================
# APPLICATION ROUTER
# =====================================================

# Initialize session state
if "page" not in st.session_state:
    st.session_state.page = "home"

if "confirm_clear" not in st.session_state:
    st.session_state.confirm_clear = False

# Route to appropriate page
page = st.session_state.page

if page == "home":
    landing_page()
elif page == "login":
    login_page()
elif page == "dashboard":
    dashboard_page()
else:
    st.session_state.page = "home"
    st.rerun()